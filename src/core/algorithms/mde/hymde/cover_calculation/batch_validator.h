#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/mde/hymde/compact_mde_storage.h"
#include "algorithms/mde/hymde/cover_calculation/invalidated_rhss.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/rhs.h"
#include "algorithms/mde/hymde/cover_calculation/mde_element.h"
#include "algorithms/mde/hymde/cover_calculation/recommendation.h"
#include "algorithms/mde/hymde/cover_calculation/validation_selection.h"
#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_lr_map.h"
#include "algorithms/mde/hymde/record_match_indexes/upper_set_index.h"
#include "algorithms/mde/hymde/record_match_indexes/value_matrix.h"
#include "model/index.h"
#include "util/bitset_utils.h"
#include "util/desbordante_assume.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::cover_calculation {
class ValidatedAdder {
    std::vector<SameLhsMDEsSpecification>& mde_specifications_;
    std::vector<model::Index> const& rm_translation_;
    std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps_;

public:
    ValidatedAdder(std::vector<SameLhsMDEsSpecification>& mde_specifications,
                   std::vector<model::Index> const& rm_translation,
                   std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps)
        : mde_specifications_(mde_specifications),
          rm_translation_(rm_translation),
          rcv_id_lr_maps_(rcv_id_lr_maps) {}

    void AddMDEs(ValidationSelection& selection, InvalidatedRhss const& invalidated,
                 std::size_t support) {
        lattice::MdeLhs const& lattice_lhs = selection.updater->GetLhs();
        lattice::Rhs const& rhs = selection.updater->GetRhs();
        std::vector<RecordClassifierSpecification> lhs_rm_specification =
                util::GetPreallocatedVector<RecordClassifierSpecification>(
                        lattice_lhs.Cardinality());
        model::Index total_offset = 0;
        for (auto const& [offset, lhs_rcv_id] : lattice_lhs) {
            total_offset += offset;
            RecordClassifierValueId rcv_id =
                    rcv_id_lr_maps_[total_offset].lhs_to_rhs_map[lhs_rcv_id];
            model::Index const translated_rm_index = rm_translation_[total_offset];
            lhs_rm_specification.emplace_back(translated_rm_index, rcv_id);
            ++total_offset;
        }

        std::vector<RecordClassifierSpecification> rhs_rm_specification;
        for (auto const& [index, rcv_id] : invalidated.GetUpdateView()) {
            selection.rhs_indices_to_validate.set(index, false);
            if (rcv_id == kLowestRCValueId) continue;
            model::Index const translated_rm_index = rm_translation_[index];
            rhs_rm_specification.emplace_back(translated_rm_index, rcv_id);
        }
        util::ForEachIndex(selection.rhs_indices_to_validate, [&](model::Index index) {
            model::Index const translated_rm_index = rm_translation_[index];
            RecordClassifierValueId const rcv_id = rhs[index];
            assert(rcv_id != kLowestRCValueId);
            rhs_rm_specification.emplace_back(translated_rm_index, rcv_id);
        });
        if (rhs_rm_specification.empty()) return;
        mde_specifications_.emplace_back(LhsSpecification{std::move(lhs_rm_specification), support},
                                         std::move(rhs_rm_specification));
    }
};

class BatchValidator {
public:
    using OneRhsRecommendations = std::vector<Recommendation>;
    using AllRhsRecommendations = std::vector<OneRhsRecommendations>;

    // We're actually doing three distinct tasks here:
    struct Result {
        // Collecting RHSs that are invalid to refine our assumptions (i.e. actual validation);
        InvalidatedRhss invalidated_rhss;
        // Collecting prospective pairs;
        AllRhsRecommendations all_rhs_recommendations;
        // Determining LHS support.
        std::size_t support = 0;
    };

private:
    using RemovedAndInterestingnessRCVIds =
            std::pair<std::vector<RecordClassifierValueId>, std::vector<RecordClassifierValueId>>;
    using RecPtr = record_match_indexes::PartitionIndex::Clusters const*;
    using RecordCluster = std::vector<RecPtr>;

    template <typename PartitionElementProvider>
    class LHSMRPartitionInspector;
    class OneCardPartitionElementProvider;
    class MultiCardPartitionElementProvider;

    enum class MdValidationStatus { kInvalidated, kCheckedAll };

    class RhsValidator {
        using LeftValueGroupedLhsRecords = std::unordered_map<PartitionValueId, RecordCluster>;

        OneRhsRecommendations* recommendations_;
        MdeElement old_rhs_;
        RecordClassifierValueId current_rcv_id_;

        // Optimize creation of LHS records groups.
        std::size_t rhs_col_match_values_;

        // Either the RHS RCV ID corresponding to the LHS RCV ID of the record match, or the
        // greatest RHS RCV ID among generalizations, whichever is greater. If `current_rcv_id`
        // reaches this value, the MDE should be removed from the lattice as either trivial or
        // non-minimal.
        RecordClassifierValueId interestingness_rcv_id_;

        record_match_indexes::PartitionIndex::RecordClustersMapping const* right_clusters_;
        record_match_indexes::ValueMatrix const* similarity_matrix_;

        model::Index record_match_index_;

        void AddRecommendations(
                RecordCluster const& same_left_value_records,
                record_match_indexes::PartitionIndex::Clusters const& right_record) {
            for (RecPtr left_record_ptr : same_left_value_records) {
                recommendations_->push_back({left_record_ptr, &right_record});
            }
        }

        void RhsIsInvalid(RecordCluster const& same_left_value_records,
                          record_match_indexes::PartitionIndex::Clusters const& right_record) {
            current_rcv_id_ = kLowestRCValueId;
            AddRecommendations(same_left_value_records, right_record);
        }

        LeftValueGroupedLhsRecords GroupLhsRecords(RecordCluster const& lhs_records) const {
            LeftValueGroupedLhsRecords left_value_grouped_lhs_records(
                    std::min(lhs_records.size(), rhs_col_match_values_));
            for (RecPtr left_record_ptr : lhs_records) {
                left_value_grouped_lhs_records[(*left_record_ptr)[record_match_index_]].push_back(
                        left_record_ptr);
            }
            return left_value_grouped_lhs_records;
        }

        // NOTE: in this implementation, pairs are only added as recommendations if they invalidate
        // the RHS.
        // TODO: test performance when records that lower the RCV ID are added as recommendations as
        // well.
        bool LowerRCVID(PartitionValueId left_pvalue_id,
                        record_match_indexes::PartitionIndex::Clusters const& right_record) {
            record_match_indexes::ValueMatrixRow const& left_value_value_mapping =
                    (*similarity_matrix_)[left_pvalue_id];

            PartitionValueId const right_value_id = right_record[record_match_index_];

            auto value_mapping_iter = left_value_value_mapping.find(right_value_id);
            if (value_mapping_iter == left_value_value_mapping.end()) {
                return true;
            }

            RecordClassifierValueId const pair_rcv_id = value_mapping_iter->second;
            if (pair_rcv_id < current_rcv_id_) {
                current_rcv_id_ = pair_rcv_id;
                if (pair_rcv_id == interestingness_rcv_id_) {
                    return true;
                }
            }
            DESBORDANTE_ASSUME(interestingness_rcv_id_ < current_rcv_id_);
            return false;
        }

    public:
        RhsValidator(
                MdeElement const old_rhs, OneRhsRecommendations& recommendations,
                std::size_t const col_match_values,
                RecordClassifierValueId const interestingness_id,
                record_match_indexes::PartitionIndex::RecordClustersMapping const& right_clusters,
                record_match_indexes::ValueMatrix const& similarity_matrix,
                model::Index const record_match_index) noexcept
            : recommendations_(&recommendations),
              old_rhs_(old_rhs),
              current_rcv_id_(old_rhs.rcv_id),
              rhs_col_match_values_(col_match_values),
              interestingness_rcv_id_(interestingness_id),
              right_clusters_(&right_clusters),
              similarity_matrix_(&similarity_matrix),
              record_match_index_(record_match_index) {}

        template <typename Collection>
        MdValidationStatus LowerRCVIDAndCollectRecommendations(
                RecordCluster const& lhs_records, Collection const& matched_rhs_records);

        void AddIfInvalid(InvalidatedRhss& invalidated) const {
            DESBORDANTE_ASSUME(current_rcv_id_ <= old_rhs_.rcv_id);
            if (current_rcv_id_ != old_rhs_.rcv_id) invalidated.PushBack(old_rhs_, current_rcv_id_);
        }

        MdeElement GetOldRhs() const noexcept {
            return old_rhs_;
        }
    };

    using SameLhsValidators = std::vector<RhsValidator>;
    using CurrentValidatorBatch = std::vector<SameLhsValidators>;

    CurrentValidatorBatch current_validator_batch_;
    std::vector<Result> results_;
    util::WorkerThreadPool* pool_;
    record_match_indexes::DataPartitionIndex const* const data_partition_index_;
    std::vector<record_match_indexes::Indexes> const* const record_match_indexes_;
    std::size_t const min_support_;
    lattice::MdeLattice* const lattice_;
    ValidatedAdder validated_adder_;

    record_match_indexes::PartitionIndex const& GetLeftPartitionIndex() const {
        return data_partition_index_->GetLeft();
    }

    record_match_indexes::PartitionIndex const& GetRightPartitionIndex() const {
        return data_partition_index_->GetRight();
    }

    std::size_t GetLeftTablePliValueNum(model::Index const record_match_index) const {
        return GetLeftPartitionIndex().GetPli(record_match_index).size();
    }

    std::size_t GetTotalPairsNum() const noexcept {
        return data_partition_index_->GetPairsNumber();
    }

    [[nodiscard]] record_match_indexes::UpperSet const* GetSimilarRecords(
            model::Index record_match_index, PartitionValueId pvalue_id,
            model::Index lhs_rcv_id) const;

    void FillValidators(SameLhsValidators& same_lhs_validators,
                        AllRhsRecommendations& recommendations,
                        std::vector<model::Index> const& pending_rhs_indices,
                        RemovedAndInterestingnessRCVIds const& removed_and_interestingness) const;
    RemovedAndInterestingnessRCVIds GetRemovedAndInterestingness(
            ValidationSelection const& info, std::vector<model::Index> const& indices);
    void CreateValidators(ValidationSelection const& info, SameLhsValidators& working,
                          AllRhsRecommendations& recommendations);

    void ValidateEmptyLhs(Result& result, boost::dynamic_bitset<> const& indices_bitset,
                          lattice::Rhs const& lattice_rhs) const;
    void RemoveTrivialForCardinality1Lhs(model::Index lhs_index, Result& result,
                                         boost::dynamic_bitset<>& indices_bitset,
                                         lattice::Rhs const& lattice_rhs);
    template <bool PerformValidation>
    decltype(auto) GetRhsValidatorsContainer();
    template <bool PerformValidation>
    void CreateResult(ValidationSelection& validation_info);
    template <bool PerformValidation>
    void CreateResults(std::vector<ValidationSelection>& all_validation_info);
    void Validate(ValidationSelection& validation_info, Result& result,
                  SameLhsValidators& working) const;

public:
    BatchValidator(util::WorkerThreadPool* pool,
                   record_match_indexes::DataPartitionIndex const* const data_partition_index,
                   std::vector<record_match_indexes::Indexes> const& record_match_indexes,
                   std::size_t min_support, lattice::MdeLattice* lattice,
                   ValidatedAdder validated_adder)
        : pool_(pool),
          data_partition_index_(data_partition_index),
          record_match_indexes_(&record_match_indexes),
          min_support_(min_support),
          lattice_(lattice),
          validated_adder_(std::move(validated_adder)) {}

    std::vector<Result> const& ValidateBatch(
            std::vector<ValidationSelection>& minimal_lhs_mds_batch);

    [[nodiscard]] bool Supported(std::size_t support) const noexcept {
        return support >= min_support_;
    }
};
}  // namespace algos::hymde::cover_calculation
