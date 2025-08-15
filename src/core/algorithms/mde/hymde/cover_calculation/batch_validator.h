#pragma once

#include <cstddef>
#include <ranges>
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
    std::vector<model::Index> const& reordered_to_original_rm_map_;
    std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps_;

    std::vector<RecordClassifierIdentifiers> MakeLhsRCIdentifierArray(
            lattice::PathToNode const& lattice_lhs) const;
    void ResetBitsForRemoved(boost::dynamic_bitset<>& selected_rhs_indices,
                             ValidationRhsUpdates const& invalidated) const;
    std::vector<RecordClassifierIdentifiers> MakeRhsRCIdentifierArray(
            ValidationRhsUpdates const& invalidated,
            boost::dynamic_bitset<>& rhs_indices_to_validate,
            std::size_t dependencies_number_for_lhs, lattice::Rhs const& rhs) const;

public:
    ValidatedAdder(std::vector<SameLhsMDEsSpecification>& mde_specifications,
                   std::vector<model::Index> const& rm_translation,
                   std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps)
        : mde_specifications_(mde_specifications),
          reordered_to_original_rm_map_(rm_translation),
          rcv_id_lr_maps_(rcv_id_lr_maps) {}

    void AddMDEs(ValidationSelection& selection, ValidationRhsUpdates const& invalidated,
                 std::size_t support);
};

class BatchValidator {
public:
    // We're actually doing three distinct tasks here ("validation" may be a misnomer):
    struct Result {
        // Collecting RHSs that are invalid to refine our assumptions (i.e. actual validation);
        ValidationRhsUpdates invalidated_rhss;
        // Collecting pairs that contradict our initial assumption;
        LhsGroupedRecommendations lhs_grouped_recommendations;
        // Determining LHS support.
        // NOTE: if all RHSs are invalid, then it only matters whether the support is below the
        // threshold or not.
        std::size_t support = 0;
        // What's a better name? Too generic: we are traversing record pairs.
        // In this traversal we are testing an assumption for several dependencies that share a LHS.
        // We initially assume that they are part of the minimal cover of the set of dependencies
        // that hold on the input data. This traversal can tell us that they are not a part of that
        // set but also determine that there is another dependency that is but has a lower RCV in
        // its RHS. "minimality assumption tester"? Minimality where? The assumption is not just
        // minimality, but that it also holds. The assumption is that the dependency is in a set.
        // The set is a cover of another set. "minimal holding assumption evaluator"

        // Too generic: we are collecting evidence pairs.
        // Each evidence pair can tell us that a weaker LHS exists that lets the RHS RCV be just as
        // high (and so can be removed).
        // But some evidence is not useful for the purpose that it is going to be used.

        // A validator takes an assumption and presents the truth and some evidence towards the
        // initial assumption's falsity. What is the meaning of support here, then?
        // The evidence is used to refine all assumptions in a more efficient way.
        // Validation checks an assumption, pair inference checks all assumptions against a pair.

        // Support is a property of a LHS on the input data.
        // The process that is happening here is about properties of a dependency's LHS? One
        // property is "assumption holds+evidence to the contrary", the other is the value of
        // support.

        // What is something that can determine a property?
    };

private:
    using RemovedAndInterestingnessRCVIds =
            std::pair<std::vector<RecordClassifierValueId>, std::vector<RecordClassifierValueId>>;

    // TODO: use plain pointer?
    using PValueIdMapPtr = record_match_indexes::PartitionIndex::PartitionValueIdMap const*;
    // Represents a collection of PartitionValueIdMap for records in a partition element of the left
    // table (either SLTVPE partition or SLTV partition).
    using LTPVsPEPVIdMaps = std::vector<PValueIdMapPtr>;

    template <typename PartitionElementProvider>
    class LHSMRPartitionInspector;

    class OneCardPartitionElementProvider;
    class MultiCardPartitionElementProvider;

    class RhsValidator {
        // Recommend if invalidated or below old threshold? Or recommend only if invalidated?
        // Currently using the former.
        // TODO: test performance for both these options.
        enum class ValidationStatus {
            kDepNotInteresting,    // Stop validation process for this RHS.
            kDepStillInteresting,  // Keep going.
        };
        // Alternative:
        // - kDepNotInteresting (RHS RCV ID below interestingness threshold, should add
        // recommendation)
        // - kBelowOldThreshold (RHS RCV ID below the old threshold, may add recommendation)
        // - kOldDepHolds (RHS RCV ID not below the old threshold, not an interesting case)
        // Validation for this RHS is stopped once some condition holds like if enough pairs are
        // recommended, not right after it is not interesting anymore.

        // Record match to partition value ID maps for records from an element of a partition of the
        // left table grouped by the partitioning value returned by this object's RHS's left table
        // partitioning function.
        using RhsLTPVGroupedLTPVsPEPVIdMaps = std::unordered_map<PartitionValueId, LTPVsPEPVIdMaps>;

        MdeElement old_rhs_;
        RecordClassifierValueId current_rcv_id_;

        // Optimize creation of left table record groups.
        std::size_t rhs_lt_pvalues_count_;

        // Either the RHS RCV ID corresponding to the LHS RCV ID of the record match, or the
        // greatest RHS RCV ID among generalizations, whichever is greater. If `current_rcv_id`
        // reaches this value, the MDE should be removed from the lattice as either trivial or
        // non-minimal.
        RecordClassifierValueId interestingness_rcv_id_;

        record_match_indexes::ValueMatrix const* value_matrix_;

        LhsGroupedRecommendations* recommendations_;

        RhsLTPVGroupedLTPVsPEPVIdMaps grouped_ltvpvs_pvid_maps_;

        // NOTE: in this implementation, pairs are only added as recommendations if they
        // invalidate the RHS.
        // kBelowThreshold is only useful if recommendations are added when a pair does not
        // invalidate it.
        ValidationStatus LowerRCVID(
                PartitionValueId lt_pvalue_id,
                record_match_indexes::PartitionIndex::PartitionValueIdMap const& rt_pvid_map) {
            record_match_indexes::ValueMatrixRow const& lt_pvalue_vm_row =
                    (*value_matrix_)[lt_pvalue_id];

            PartitionValueId const rt_pvalue_id = rt_pvid_map[old_rhs_.record_match_index];

            auto rcv_id_iter = lt_pvalue_vm_row.find(rt_pvalue_id);
            if (rcv_id_iter == lt_pvalue_vm_row.end()) {
                current_rcv_id_ = kLowestRCValueId;
                return ValidationStatus::kDepNotInteresting;
            }

            RecordClassifierValueId const rcv_id = rcv_id_iter->second;
            if (rcv_id < current_rcv_id_) {
                if (rcv_id == interestingness_rcv_id_) {
                    return ValidationStatus::kDepNotInteresting;
                }
                current_rcv_id_ = rcv_id;
            }
            DESBORDANTE_ASSUME(interestingness_rcv_id_ < current_rcv_id_);
            return ValidationStatus::kDepStillInteresting;
        }

    public:
        RhsValidator(MdeElement const old_rhs, LhsGroupedRecommendations& recommendations,
                     std::size_t const rhs_lt_pvalues_count,
                     RecordClassifierValueId const interestingness_id,
                     record_match_indexes::ValueMatrix const& value_matrix) noexcept
            : old_rhs_(old_rhs),
              current_rcv_id_(old_rhs.rcv_id),
              rhs_lt_pvalues_count_(rhs_lt_pvalues_count),
              interestingness_rcv_id_(interestingness_id),
              value_matrix_(&value_matrix),
              recommendations_(&recommendations) {}

        void SetCurrentLTPVsPElement(LTPVsPEPVIdMaps const& ltpvspe_pvid_maps) {
            grouped_ltvpvs_pvid_maps_.clear();
            grouped_ltvpvs_pvid_maps_.reserve(
                    std::min(ltpvspe_pvid_maps.size(), rhs_lt_pvalues_count_));
            for (PValueIdMapPtr left_table_pvid_map_ptr : ltpvspe_pvid_maps) {
                grouped_ltvpvs_pvid_maps_[(*left_table_pvid_map_ptr)[old_rhs_.record_match_index]]
                        .push_back(left_table_pvid_map_ptr);
            }
        }

        // Returns whether validation must stop for this RHS. If `true` is returned, the RHS must
        // have been invalidated.
        bool CheckRecord(
                record_match_indexes::PartitionIndex::PartitionValueIdMap const& rt_pvid_map) {
            for (auto& [rhs_lt_pvid, rhs_lt_pvid_maps] : grouped_ltvpvs_pvid_maps_) {
                ValidationStatus status = LowerRCVID(rhs_lt_pvid, rt_pvid_map);
                if (status == ValidationStatus::kDepNotInteresting) {
                    // Due to this move, this method must not be called once RHS is invalidated.
                    recommendations_->emplace_back(std::move(rhs_lt_pvid_maps), &rt_pvid_map);
                    return true;
                }
            }
            return false;
        }

        void AddIfInvalid(ValidationRhsUpdates& invalidated) const {
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
    std::vector<record_match_indexes::RcvIdLRMap> const* rcv_id_lr_maps_;
    ValidatedAdder validated_adder_;

    record_match_indexes::PartitionIndex const& GetLeftTablePartitionIndex() const {
        return data_partition_index_->GetLeft();
    }

    record_match_indexes::PartitionIndex const& GetRightTablePartitionIndex() const {
        return data_partition_index_->GetRight();
    }

    std::size_t GetLeftTablePartitioningValuesCount(model::Index const record_match_index) const {
        return GetLeftTablePartitionIndex().GetPli(record_match_index).size();
    }

    std::size_t GetTotalPairsNum() const noexcept {
        return data_partition_index_->GetPairsNumber();
    }

    bool SingleThreaded() const noexcept {
        return pool_ == nullptr;
    }

    void FillValidators(SameLhsValidators& same_lhs_validators,
                        LhsGroupedRecommendations& recommendations,
                        std::vector<model::Index> const& pending_rhs_indices,
                        RemovedAndInterestingnessRCVIds const& removed_and_interestingness) const;
    RemovedAndInterestingnessRCVIds GetRemovedAndInterestingness(
            ValidationSelection const& selection, std::vector<model::Index> const& indices);
    void CreateValidators(ValidationSelection const& selection, SameLhsValidators& working,
                          LhsGroupedRecommendations& recommendations);

    void ValidateEmptyLhs(Result& result, boost::dynamic_bitset<> const& indices_bitset,
                          lattice::Rhs const& lattice_rhs) const;
    void RemoveTrivialForCardinality1Lhs(model::Index lhs_index, Result& result,
                                         boost::dynamic_bitset<>& indices_bitset,
                                         lattice::Rhs const& lattice_rhs);
    template <bool PerformValidation>
    decltype(auto) GetRhsValidatorsContainer();
    template <bool PerformValidation>
    void CreateResult(ValidationSelection& selection);
    template <bool PerformValidation>
    void CreateResults(std::vector<ValidationSelection>& all_validation_info);
    void Validate(ValidationSelection& selection, Result& result, SameLhsValidators& working) const;

public:
    BatchValidator(util::WorkerThreadPool* pool,
                   record_match_indexes::DataPartitionIndex const* const data_partition_index,
                   std::vector<record_match_indexes::Indexes> const& record_match_indexes,
                   std::size_t min_support, lattice::MdeLattice* lattice,
                   std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps,
                   ValidatedAdder validated_adder);

    [[nodiscard]] bool Supported(std::size_t support) const noexcept {
        return support >= min_support_;
    }

    std::vector<Result> const& ValidateBatch(
            std::vector<ValidationSelection>& minimal_lhs_mds_batch);

    std::vector<Result> const& GetCurrentResults() const noexcept {
        return results_;
    }
};
}  // namespace algos::hymde::cover_calculation
