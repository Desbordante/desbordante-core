#pragma once

#include <cstddef>
#include <unordered_set>
#include <vector>

#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/lattice/validation_info.h"
#include "algorithms/md/hymd/recommendation.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "algorithms/md/hymd/utility/invalidated_rhss.h"
#include "model/index.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd {

class BatchValidator {
public:
    using OneRhsRecommendations = std::vector<Recommendation>;
    using AllRhsRecommendations = std::vector<OneRhsRecommendations>;

    // We're actually doing three distinct tasks here:
    struct Result {
        // Collecting RHSs that are invalid to refine our assumptions (i.e. actual validation);
        utility::InvalidatedRhss invalidated_rhss;
        // Collecting prospective pairs;
        AllRhsRecommendations all_rhs_recommendations;
        // Checking if the LHS has enough support.
        bool lhs_is_unsupported;
    };

private:
    using RemovedAndInterestingnessCCVIds =
            std::pair<std::vector<ColumnClassifierValueId>, std::vector<ColumnClassifierValueId>>;
    using RecPtr = CompressedRecord const*;
    using RecordCluster = std::vector<RecPtr>;

    template <typename PartitionElementProvider>
    class LHSMRPartitionInspector;
    class OneCardPartitionElementProvider;
    class MultiCardPartitionElementProvider;

    enum class MdValidationStatus { kInvalidated, kCheckedAll };

    class RhsValidator {
        using LeftValueGroupedLhsRecords = std::unordered_map<ValueIdentifier, RecordCluster>;

        OneRhsRecommendations* recommendations_;
        MdElement old_rhs_;
        ColumnClassifierValueId current_ccv_id_;

        // Optimize creation of LHS records groups.
        std::size_t rhs_col_match_values_;

        // Either the RHS CCV ID corresponding to the LHS CCV ID of the column match, or the
        // greatest RHS CCV ID among generalizations, whichever is greater. If `current_ccv_id`
        // reaches this value, the MD should be removed from the lattice as either trivial or
        // non-minimal.
        ColumnClassifierValueId interestingness_ccv_id_;

        indexes::CompressedRecords const* right_records_;
        indexes::SimilarityMatrix const* similarity_matrix_;

        // Indices of the column from the left table and the right table for the RHS column
        // classifier of the MD being validated.
        model::Index left_column_index_;
        model::Index right_column_index_;

        void AddRecommendations(RecordCluster const& same_left_value_records,
                                CompressedRecord const& right_record) {
            for (RecPtr left_record_ptr : same_left_value_records) {
                recommendations_->emplace_back(left_record_ptr, &right_record);
            }
        }

        void RhsIsInvalid(RecordCluster const& same_left_value_records,
                          CompressedRecord const& right_record) {
            current_ccv_id_ = kLowestCCValueId;
            for (RecPtr left_record_ptr : same_left_value_records) {
                recommendations_->emplace_back(left_record_ptr, &right_record);
            }
        }

        LeftValueGroupedLhsRecords GroupLhsRecords(RecordCluster const& lhs_records) const {
            LeftValueGroupedLhsRecords left_value_grouped_lhs_records(
                    std::min(lhs_records.size(), rhs_col_match_values_));
            for (RecPtr left_record_ptr : lhs_records) {
                left_value_grouped_lhs_records[(*left_record_ptr)[left_column_index_]].push_back(
                        left_record_ptr);
            }
            return left_value_grouped_lhs_records;
        }

        // NOTE: in this implementation, pairs are only added as recommendations if they invalidate
        // the RHS.
        // TODO: test performance when records that lower the CCV ID are added as recommendations as
        // well.
        bool LowerCCVID(ValueIdentifier left_column_value_id,
                        CompressedRecord const& right_record) {
            indexes::SimilarityMatrixRow const& left_value_value_mapping =
                    (*similarity_matrix_)[left_column_value_id];

            ValueIdentifier const right_value_id = right_record[right_column_index_];

            auto value_mapping_iter = left_value_value_mapping.find(right_value_id);
            if (value_mapping_iter == left_value_value_mapping.end()) {
                return true;
            }

            ColumnClassifierValueId const pair_ccv_id = value_mapping_iter->second;
            if (pair_ccv_id < current_ccv_id_) {
                current_ccv_id_ = pair_ccv_id;
                if (pair_ccv_id == interestingness_ccv_id_) {
                    return true;
                }
            }
            DESBORDANTE_ASSUME(interestingness_ccv_id_ < current_ccv_id_);
            return false;
        }

    public:
        RhsValidator(MdElement const old_rhs, OneRhsRecommendations& recommendations,
                     std::size_t const col_match_values,
                     ColumnClassifierValueId const interestingness_id,
                     indexes::CompressedRecords const& right_records,
                     indexes::SimilarityMatrix const& similarity_matrix,
                     model::Index const left_column_index,
                     model::Index const right_column_index) noexcept
            : recommendations_(&recommendations),
              old_rhs_(old_rhs),
              current_ccv_id_(old_rhs.ccv_id),
              rhs_col_match_values_(col_match_values),
              interestingness_ccv_id_(interestingness_id),
              right_records_(&right_records),
              similarity_matrix_(&similarity_matrix),
              left_column_index_(left_column_index),
              right_column_index_(right_column_index) {}

        template <typename Collection>
        MdValidationStatus LowerCCVIDAndCollectRecommendations(
                RecordCluster const& lhs_records, Collection const& matched_rhs_records);

        void AddIfInvalid(utility::InvalidatedRhss& invalidated) const {
            DESBORDANTE_ASSUME(current_ccv_id_ <= old_rhs_.ccv_id);
            if (current_ccv_id_ != old_rhs_.ccv_id) invalidated.PushBack(old_rhs_, current_ccv_id_);
        }

        MdElement GetOldRhs() const noexcept {
            return old_rhs_;
        }
    };

    using SameLhsValidators = std::vector<RhsValidator>;
    using CurrentValidatorBatch = std::vector<SameLhsValidators>;

    CurrentValidatorBatch current_validator_batch_;
    std::vector<Result> results_;
    util::WorkerThreadPool* pool_;
    indexes::RecordsInfo const* const records_info_;
    std::vector<ColumnMatchInfo> const* const column_matches_info_;
    std::size_t const min_support_;
    lattice::MdLattice* const lattice_;

    [[nodiscard]] bool Supported(std::size_t support) const noexcept {
        return support >= min_support_;
    }

    [[nodiscard]] model::Index GetLeftPliIndex(model::Index const column_match_index) const {
        return (*column_matches_info_)[column_match_index].left_column_index;
    }

    indexes::DictionaryCompressor const& GetLeftCompressor() const noexcept {
        return records_info_->GetLeftCompressor();
    }

    indexes::DictionaryCompressor const& GetRightCompressor() const noexcept {
        return records_info_->GetRightCompressor();
    }

    std::size_t GetLeftTableColumnValueNum(model::Index const column_match_index) const {
        return GetLeftCompressor().GetPli(GetLeftPliIndex(column_match_index)).GetClusters().size();
    }

    std::size_t GetTotalPairsNum() const noexcept {
        return records_info_->GetTotalPairsNum();
    }

    [[nodiscard]] indexes::RecSet const* GetSimilarRecords(model::Index column_match_index,
                                                           ValueIdentifier value_id,
                                                           model::Index lhs_ccv_id) const;

    void FillValidators(SameLhsValidators& same_lhs_validators,
                        AllRhsRecommendations& recommendations,
                        std::vector<model::Index> const& pending_rhs_indices,
                        RemovedAndInterestingnessCCVIds const& removed_and_interestingness) const;
    RemovedAndInterestingnessCCVIds GetRemovedAndInterestingness(
            lattice::ValidationInfo const& info, std::vector<model::Index> const& indices);
    void CreateValidators(lattice::ValidationInfo const& info, SameLhsValidators& working,
                          AllRhsRecommendations& recommendations);

    void ValidateEmptyLhs(Result& result, boost::dynamic_bitset<> const& indices_bitset,
                          lattice::Rhs const& lattice_rhs) const;
    void RemoveTrivialForCardinality1Lhs(model::Index lhs_index, Result& result,
                                         boost::dynamic_bitset<>& indices_bitset,
                                         lattice::Rhs const& lattice_rhs);
    template <bool PerformValidation>
    decltype(auto) GetRhsValidatorsContainer();
    template <bool PerformValidation>
    void CreateResult(lattice::ValidationInfo& validation_info);
    template <bool PerformValidation>
    void CreateResults(std::vector<lattice::ValidationInfo>& all_validation_info);
    void Validate(lattice::ValidationInfo& validation_info, Result& result,
                  SameLhsValidators& working) const;

public:
    BatchValidator(util::WorkerThreadPool* pool, indexes::RecordsInfo const* records_info,
                   std::vector<ColumnMatchInfo> const& column_matches_info, std::size_t min_support,
                   lattice::MdLattice* lattice)
        : pool_(pool),
          records_info_(records_info),
          column_matches_info_(&column_matches_info),
          min_support_(min_support),
          lattice_(lattice) {}

    std::vector<Result> const& ValidateBatch(
            std::vector<lattice::ValidationInfo>& minimal_lhs_mds_batch);
};

}  // namespace algos::hymd
