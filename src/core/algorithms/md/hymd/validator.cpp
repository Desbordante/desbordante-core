#include "algorithms/md/hymd/validator.h"

#include <optional>
#include <span>
#include <vector>

#include <boost/core/pointer_traits.hpp>

#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "algorithms/md/hymd/utility/invalidated_rhss.h"
#include "algorithms/md/hymd/utility/reserve_more.h"
#include "algorithms/md/hymd/utility/trivial_array.h"
#include "algorithms/md/hymd/utility/zip.h"
#include "md/hymd/column_match_info.h"
#include "md/hymd/indexes/column_similarity_info.h"
#include "md/hymd/indexes/pli_cluster.h"
#include "md/hymd/lattice/md_lattice.h"
#include "md/hymd/lattice/validation_info.h"
#include "md/hymd/md_lhs.h"
#include "model/index.h"
#include "util/bitset_utils.h"
#include "util/erase_if_replace.h"
#include "util/get_preallocated_vector.h"
#include "worker_thread_pool.h"

namespace algos::hymd {
using indexes::CompressedRecords;
using indexes::PliCluster;
using indexes::RecSet;
using model::Index;
using IndexVector = std::vector<Index>;
using RecIdVec = std::vector<RecordIdentifier>;

// There are several partitions that are going to be mentioned here.
// Firstly, a partition of the left table based on equality of values of some set of attributes
// (`LTVsPartition` in code, from "left table values partition"). Note that we already create a
// partition for each attribute in the preprocessing stage (`SLTVPartition` here, from "single left
// table value partition"), which is represented as a PLI for each column. Thus, for any partition
// where the key contains multiple attributes we can obtain the sets of the partition with multiple
// attributes in its key by partitioning each set of any attribute "properly" only on the other
// attributes (`SLTVPEPartition`, "single left table value partition element partition").

// Finally, partition of the set of record pairs that are matched by an LHS (`LHSMRPartition`, "LHS
// matched records partition"). Its sets can be obtained by inspecting each set of the first
// partition with the partition key being a set of the left table columns of the LHS's similarity
// classifiers (if \psi is the set of column classifiers with non-zero decision boundaries of the
// LHS, then the set is {l | \exists r,sim,value ((l, r, sim), value) \in \psi}). Along with the
// partition values, each left table's record in this set also shares the set of right table's
// records that are matched by the LHS to it with the others, if any. The Cartesian products of the
// described sets are the elements of the second partition.

// This class represents an element of the LHSMRPartition, containing the factors for each
// Cartesian product.
template <typename LhsRecordIdsType, typename RhsRecordIdsType>
struct LHSMRPartitionElement {
    LhsRecordIdsType const& lhs_records;
    RhsRecordIdsType const& matched_rhs_records;

    std::size_t GetPairsNumber() const noexcept {
        return lhs_records.size() * matched_rhs_records.size();
    }
};

RecSet const* BatchValidator::GetSimilarRecords(Index const column_match_index,
                                                ValueIdentifier const value_id,
                                                model::Index const lhs_ccv_id) const {
    indexes::SimilarityIndex const& similarity_index =
            (*column_matches_info_)[column_match_index].similarity_info.similarity_index;
    indexes::ValueUpperSetMapping const& mapping = similarity_index[value_id];
    // Upper sets for zero CCV IDs are the entire right table and should be neither stored nor
    // requested.
    DESBORDANTE_ASSUME(lhs_ccv_id != kLowestCCValueId);
    return mapping.GetUpperSet(lhs_ccv_id);
}

template <typename LHSMRPartitionElementProvider>
class BatchValidator::LHSMRPartitionInspector {
    using PartitionElement = LHSMRPartitionElementProvider::PartitionElement;

    BatchValidator const* const validator_;
    CompressedRecords const& left_records_ = validator_->GetLeftCompressor().GetRecords();
    Result& result_;
    SameLhsValidators& rhs_validators_;
    LHSMRPartitionElementProvider lhsmr_partition_element_provider_;
    std::size_t lhs_support_ = 0;

    [[nodiscard]] MdValidationStatus LowerCCVIDsAndCollectPairs(
            RhsValidator& rhs_validator, PliCluster const& cluster,
            RecSet const& similar_records) const;
    [[nodiscard]] MdValidationStatus LowerCCVIDsAndCollectPairs(
            RhsValidator& rhs_validator, RecordCluster const& matched_records,
            RecIdVec const& similar_records) const;

    bool LhsIsSupported() {
        return validator_->Supported(lhs_support_);
    }

    void MakeOutOfClustersResult() {
        if (!LhsIsSupported()) {
            result_.lhs_is_unsupported = true;
            return;
        }

        result_.lhs_is_unsupported = false;
        // If we ran out of clusters, then it means some RHSs were not invalidated and their
        // corresponding validators were not removed.
        DESBORDANTE_ASSUME(!rhs_validators_.empty());
        for (RhsValidator const& rhs_validator : rhs_validators_) {
            rhs_validator.AddIfInvalid(result_.invalidated_rhss);
        }
    }

    void ForEachPartitionElement(auto element_action, auto finish_out_of_pairs) {
        while (true) {
            std::optional<PartitionElement> partition_element_opt =
                    lhsmr_partition_element_provider_.TryGetNextLHSMRPartitionElement();
            if (!partition_element_opt.has_value()) break;

            lhs_support_ += partition_element_opt->GetPairsNumber();

            bool const finish_early = element_action(*partition_element_opt);
            if (finish_early) return;
        }
        finish_out_of_pairs();
    }

    void MakeAllInvalidatedResult() {
        // All RHSs that were invalidated should have had their validators deleted.
        DESBORDANTE_ASSUME(rhs_validators_.empty());
        auto check_support = [&](auto&&...) {
            if (LhsIsSupported()) {
                result_.lhs_is_unsupported = false;
                return true;
            }
            return false;
        };
        if (check_support()) return;

        ForEachPartitionElement(check_support, [this]() { result_.lhs_is_unsupported = true; });
    }

    // Lower CCV IDs, collect prospective pairs, remove invalidated (i.e. with CCV ID 0) RHSs.
    bool InspectPartitionElement(PartitionElement element) {
        util::EraseIfReplace(rhs_validators_, [&](RhsValidator& rhs_validator) {
            auto const& [lhs_records, matched_rhs_records] = element;
            MdValidationStatus const status =
                    LowerCCVIDsAndCollectPairs(rhs_validator, lhs_records, matched_rhs_records);
            if (status == MdValidationStatus::kInvalidated) {
                result_.invalidated_rhss.PushBack(rhs_validator.GetOldRhs(), kLowestCCValueId);
                return true;
            }
            return false;
        });
        return rhs_validators_.empty();
    }

public:
    LHSMRPartitionInspector(BatchValidator const* validator, Result& result,
                            SameLhsValidators& rhs_validators, MdLhs const& lhs)
        : validator_(validator),
          result_(result),
          rhs_validators_(rhs_validators),
          lhsmr_partition_element_provider_(validator, lhs) {}

    void InspectLHSMRPartition() {
        auto inspect_check_all_invalid = [&](PartitionElement partition_element) {
            bool const all_invalid = InspectPartitionElement(partition_element);

            if (all_invalid) {
                MakeAllInvalidatedResult();
                return true;
            }
            return false;
        };

        ForEachPartitionElement(inspect_check_all_invalid, [this]() { MakeOutOfClustersResult(); });
    }
};

template <typename Collection>
auto BatchValidator::RhsValidator::LowerCCVIDAndCollectRecommendations(
        RecordCluster const& lhs_records,
        Collection const& matched_rhs_records) -> MdValidationStatus {
    // Invalidated are removed (1), empty LHSMR partition "elements" should have been skipped (2,
    // 3), non-minimal are considered invalidated (4).
    DESBORDANTE_ASSUME(current_ccv_id_ != kLowestCCValueId);        // 1
    DESBORDANTE_ASSUME(!matched_rhs_records.empty());               // 2
    DESBORDANTE_ASSUME(!lhs_records.empty());                       // 3
    DESBORDANTE_ASSUME(interestingness_ccv_id_ < current_ccv_id_);  // 4

    LeftValueGroupedLhsRecords grouped_lhs_records = GroupLhsRecords(lhs_records);

    for (auto const& [left_column_value_id, same_left_value_records] : grouped_lhs_records) {
        DESBORDANTE_ASSUME(matched_rhs_records.begin() != matched_rhs_records.end());
        for (RecordIdentifier rhs_record_id : matched_rhs_records) {
            CompressedRecord const& right_record = (*right_records_)[rhs_record_id];

            bool const md_invalidated = LowerCCVID(left_column_value_id, right_record);
            if (md_invalidated) {
                RhsIsInvalid(same_left_value_records, right_record);
                return MdValidationStatus::kInvalidated;
            }
        }
    }
    return MdValidationStatus::kCheckedAll;
}

template <typename LHSMRPartitionElementProvider>
auto BatchValidator::LHSMRPartitionInspector<LHSMRPartitionElementProvider>::
        LowerCCVIDsAndCollectPairs(RhsValidator& rhs_validator, RecordCluster const& lhs_records,
                                   RecIdVec const& matched_rhs_records) const
        -> MdValidationStatus {
    return rhs_validator.LowerCCVIDAndCollectRecommendations(lhs_records, matched_rhs_records);
}

template <typename LHSMRPartitionElementProvider>
auto BatchValidator::LHSMRPartitionInspector<LHSMRPartitionElementProvider>::
        LowerCCVIDsAndCollectPairs(RhsValidator& rhs_validator, PliCluster const& cluster,
                                   RecSet const& matched_rhs_records) const -> MdValidationStatus {
    // See LowerCCVIDAndCollectRecommendations comment
    DESBORDANTE_ASSUME(!matched_rhs_records.empty());

    RecordCluster cluster_records = util::GetPreallocatedVector<RecPtr>(cluster.size());

    DESBORDANTE_ASSUME(!cluster.empty());
    for (RecordIdentifier left_record_id : cluster) {
        cluster_records.push_back(&left_records_[left_record_id]);
    }
    return rhs_validator.LowerCCVIDAndCollectRecommendations(cluster_records, matched_rhs_records);
}

// Each left table column's PLI already stores the desired partition of the left table. The sets of
// records that are matched by each partition key's value are already stored in similarity indexes.
class BatchValidator::OneCardPartitionElementProvider {
    BatchValidator const* const validator_;
    ValueIdentifier current_value_id_ = ValueIdentifier(-1);
    Index const lhs_column_match_index_;
    ColumnClassifierValueId const lhs_ccv_id_;
    std::vector<PliCluster> const& clusters_ =
            validator_->GetLeftCompressor()
                    .GetPli(validator_->GetLeftPliIndex(lhs_column_match_index_))
                    .GetClusters();
    std::size_t const clusters_size_ = clusters_.size();

public:
    using PartitionElement = LHSMRPartitionElement<PliCluster, RecSet>;

    OneCardPartitionElementProvider(BatchValidator const* validator, MdLhs const& lhs)
        : validator_(validator),
          lhs_column_match_index_(lhs.begin()->offset),
          lhs_ccv_id_(lhs.begin()->ccv_id) {}

    std::optional<PartitionElement> TryGetNextLHSMRPartitionElement() {
        while (++current_value_id_ != clusters_size_) {
            RecSet const* const similar_records_ptr = validator_->GetSimilarRecords(
                    lhs_column_match_index_, current_value_id_, lhs_ccv_id_);
            if (similar_records_ptr != nullptr)
                return {{clusters_[current_value_id_], *similar_records_ptr}};
        }
        return std::nullopt;
    }
};

class BatchValidator::MultiCardPartitionElementProvider {
    // Represented by left table's column indices.
    using SLTVPEPartitionKey = std::vector<Index>;

    // Used in place of a normal SLTVPEPartition key index to indicate that the SLTVPartition value
    // should be used.
    static constexpr Index kSLTVPartitionColumn = -1;

    // Find RHS records that have a value similar with respect to `ccv_id` to
    // `sltvpe_partition_value[partition_key_index]` according to the measure of the column match at
    // index `column_match_index`.
    struct RhsRecordsMatchingCriterion {
        Index column_match_index;
        // kSLTVPartitionColumn here means we should use the SLTVPartition's partition value
        Index partition_key_index;
        ColumnClassifierValueId ccv_id;
    };

    using RhsRecordsMatchingCriteria = std::vector<RhsRecordsMatchingCriterion>;

    class Initializer {
        BatchValidator const* validator_;

        MdLhs::iterator lhs_iter_;
        MdLhs::iterator const lhs_end_;

        std::size_t const total_left_column_number_ =
                validator_->GetLeftCompressor().GetPliNumber();

        RhsRecordsMatchingCriteria rhs_records_matching_criteria_;
        SLTVPEPartitionKey sltvpe_partition_key_;

        Index cur_col_match_index_ = lhs_iter_->offset;

        // TODO: investigate impact of different columns here on performance.
        Index sltv_partition_column_index_ = validator_->GetLeftPliIndex(cur_col_match_index_);

        // Used to indicate that the SLTVPEPartition key index for a column has not been determined.
        static constexpr std::size_t kNoIndex = -2;

        std::vector<Index> left_column_to_sltv_partition_key_index_mapping_ =
                std::vector<Index>(total_left_column_number_, kNoIndex);

        std::size_t GetClusterNumber(Index left_pli_index) {
            return validator_->GetLeftCompressor().GetPli(left_pli_index).GetClusters().size();
        }

        void AddFirstRhsRecordsMatchingCriterion() {
            left_column_to_sltv_partition_key_index_mapping_[sltv_partition_column_index_] =
                    kSLTVPartitionColumn;
            // LHS has cardinality greater than 1, so is not empty.
            DESBORDANTE_ASSUME(lhs_iter_ != lhs_end_);
            rhs_records_matching_criteria_.push_back(
                    {cur_col_match_index_, kSLTVPartitionColumn, lhs_iter_->ccv_id});
            ++cur_col_match_index_;
            ++lhs_iter_;
        }

        void AddOtherRhsRecordsMatchingCriteria() {
            Index next_partition_key_index = 0;
            // LHS has cardinality greater than 1, so it has at least one more element.
            DESBORDANTE_ASSUME(lhs_iter_ != lhs_end_);
            for (auto const& [next_node_offset, ccv_id] : std::span{lhs_iter_, lhs_end_}) {
                cur_col_match_index_ += next_node_offset;
                Index const left_column_index = validator_->GetLeftPliIndex(cur_col_match_index_);
                Index& partition_key_index =
                        left_column_to_sltv_partition_key_index_mapping_[left_column_index];

                if (partition_key_index == kNoIndex) {
                    partition_key_index = next_partition_key_index++;

                    sltvpe_partition_key_.push_back(left_column_index);
                }

                rhs_records_matching_criteria_.push_back(
                        {cur_col_match_index_, partition_key_index, ccv_id});
                ++cur_col_match_index_;
            }
        }

    public:
        Initializer(BatchValidator const* validator, MdLhs const& lhs)
            : validator_(validator), lhs_iter_(lhs.begin()), lhs_end_(lhs.end()) {
            std::size_t const cardinality = lhs.Cardinality();
            // This class is adapted for this case and should only be created if this holds.
            DESBORDANTE_ASSUME(cardinality > 1);
            rhs_records_matching_criteria_.reserve(cardinality);
            sltvpe_partition_key_.reserve(std::min(cardinality, total_left_column_number_));

            AddFirstRhsRecordsMatchingCriterion();
            AddOtherRhsRecordsMatchingCriteria();
        }

        BatchValidator const* GetValidator() const noexcept {
            return validator_;
        }

        SLTVPEPartitionKey& GetPartitionKey() noexcept {
            return sltvpe_partition_key_;
        }

        Index GetSLTVPEPartitionColumnIndex() const noexcept {
            return sltv_partition_column_index_;
        }

        RhsRecordsMatchingCriteria& GetRhsRecordsMatchingCriteria() noexcept {
            return rhs_records_matching_criteria_;
        }
    };

    using ValueIdArray = utility::TrivialArray<ValueIdentifier>;
    using GroupMap = std::unordered_map<ValueIdArray, RecordCluster, ValueIdArray::Hasher,
                                        ValueIdArray::ArrEqual>;

    BatchValidator const* const validator_;

    SLTVPEPartitionKey const sltvpe_partition_key_;
    std::size_t const sltvpe_partition_key_size_ = sltvpe_partition_key_.size();
    ValueIdArray partition_value_scratch_{sltvpe_partition_key_size_};

    std::vector<PliCluster> const& sltv_partition_;
    std::size_t const sltv_partition_size_ = sltv_partition_.size();

    RhsRecordsMatchingCriteria const rhs_records_matching_criteria_;

    GroupMap sltvpe_partition_{0, ValueIdArray::Hasher(sltvpe_partition_key_size_),
                               ValueIdArray::ArrEqual(sltvpe_partition_key_size_)};
    GroupMap::iterator sltvpe_partition_iter_ = sltvpe_partition_.begin();

    ValueIdentifier sltv_partition_value_ = 0;

    std::vector<RecSet const*> matched_rhs_rec_sets_scratch_;
    CompressedRecords const& left_records_ = validator_->GetLeftCompressor().GetRecords();
    RecIdVec rhs_set_intersection_;

    MultiCardPartitionElementProvider(Initializer init_info)
        : validator_(init_info.GetValidator()),
          sltvpe_partition_key_(std::move(init_info.GetPartitionKey())),
          sltv_partition_(validator_->GetLeftCompressor()
                                  .GetPli(init_info.GetSLTVPEPartitionColumnIndex())
                                  .GetClusters()),
          rhs_records_matching_criteria_(std::move(init_info.GetRhsRecordsMatchingCriteria())) {
        matched_rhs_rec_sets_scratch_.reserve(rhs_records_matching_criteria_.size());

        // Tables are guaranteed to not be empty, so a partition can't be empty either.
        DESBORDANTE_ASSUME(sltv_partition_size_ != 0);
        CreateSLTVPEPartition();
    }

    void CreateSLTVPEPartition() {
        PliCluster const& sltv_partition_element = sltv_partition_[sltv_partition_value_];
        for (RecordIdentifier const record_id : sltv_partition_element) {
            utility::PointerArrayBackInserter inserter{partition_value_scratch_.GetFiller()};
            std::vector<ValueIdentifier> const& record = left_records_[record_id];
            for (Index const index : sltvpe_partition_key_) {
                inserter.PushBack(record[index]);
            }
            sltvpe_partition_[partition_value_scratch_.GetSpan(sltvpe_partition_key_size_)]
                    .push_back(&record);
        }
        sltvpe_partition_iter_ = sltvpe_partition_.begin();
    }

    bool NextSLTVPEPartition() {
        if (++sltv_partition_value_ == sltv_partition_size_) return false;
        // This method must not be called after false is returned.
        DESBORDANTE_ASSUME(sltv_partition_value_ < sltv_partition_size_);
        sltvpe_partition_.clear();
        CreateSLTVPEPartition();
        return true;
    }

    bool FindMatchedRhsRecordSets(ValueIdArray const& sltvpe_partition_value) {
        matched_rhs_rec_sets_scratch_.clear();
        for (auto const& [column_match_index, sltvpe_partition_key_index, ccv_id] :
             rhs_records_matching_criteria_) {
            ValueIdentifier const value_id =
                    sltvpe_partition_key_index == kSLTVPartitionColumn
                            ? sltv_partition_value_
                            : sltvpe_partition_value[sltvpe_partition_key_index];
            RecSet const* similar_records_ptr =
                    validator_->GetSimilarRecords(column_match_index, value_id, ccv_id);
            if (similar_records_ptr == nullptr) return true;
            // Empty sets shouldn't be stored in similarity indexes.
            DESBORDANTE_ASSUME(!similar_records_ptr->empty());
            matched_rhs_rec_sets_scratch_.push_back(similar_records_ptr);
        }
        return false;
    }

    void SortMatchedRhsRecordSetsBySize() {
        // Allows to loop through the smallest possible number of elements due to the smallest set
        // being placed first. As for the other sets, we can say that the smaller a set is, the more
        // likely it is that an element won't be contained in it, making us stop an iteration for an
        // element sooner on average when they are sorted by size.
        auto size_cmp = [](RecSet const* p1, RecSet const* p2) { return p1->size() < p2->size(); };
        // At least two column matches are considered, so this is impossible, use assumption to
        // avoid an extra check in std::sort.
        DESBORDANTE_ASSUME(!matched_rhs_rec_sets_scratch_.empty());
        std::ranges::sort(matched_rhs_rec_sets_scratch_, size_cmp);
    }

    void IntersectMatchedRhsRecordSets() {
        // `rhs_set_intersection_` should be treated as if it is the return value of this method. It
        // is not literally that for performance purposes (collection reuse).
        DESBORDANTE_ASSUME(rhs_set_intersection_.empty());

        SortMatchedRhsRecordSetsBySize();

        RecSet const& first = *matched_rhs_rec_sets_scratch_.front();
        std::span other_record_set_ptrs{matched_rhs_rec_sets_scratch_.begin() + 1,
                                        matched_rhs_rec_sets_scratch_.end()};
        // Empty sets have been excluded prior to the call.
        DESBORDANTE_ASSUME(first.begin() != first.end());
        for (RecordIdentifier rec : first) {
            auto contains_record = [rec](RecSet const* set_ptr) { return set_ptr->contains(rec); };
            // At least two column matches are considered.
            DESBORDANTE_ASSUME(other_record_set_ptrs.begin() != other_record_set_ptrs.end());
            if (std::ranges::all_of(other_record_set_ptrs, contains_record)) {
                rhs_set_intersection_.push_back(rec);
            }
        }
    }

public:
    using PartitionElement = LHSMRPartitionElement<RecordCluster, RecIdVec>;

    MultiCardPartitionElementProvider(BatchValidator const* validator, MdLhs const& lhs)
        : MultiCardPartitionElementProvider(Initializer{validator, lhs}) {}

    std::optional<PartitionElement> TryGetNextLHSMRPartitionElement() {
        rhs_set_intersection_.clear();
        do {
            while (sltvpe_partition_iter_ != sltvpe_partition_.end()) {
                auto const& [sltvpe_partition_value, lhs_record_ids] = *sltvpe_partition_iter_++;

                bool const empty_set_found = FindMatchedRhsRecordSets(sltvpe_partition_value);
                if (empty_set_found) continue;

                IntersectMatchedRhsRecordSets();
                if (rhs_set_intersection_.empty()) continue;

                return {{lhs_record_ids, rhs_set_intersection_}};
            }
        } while (NextSLTVPEPartition());
        return std::nullopt;
    }
};

void BatchValidator::Validate(lattice::ValidationInfo& info, Result& result,
                              SameLhsValidators& same_lhs_validators) const {
    switch (MdLhs const& lhs = info.messenger->GetLhs(); lhs.Cardinality()) {
        [[unlikely]] case 0:
            // Already validated.
            break;
        case 1: {
            LHSMRPartitionInspector<OneCardPartitionElementProvider> inspector(
                    this, result, same_lhs_validators, lhs);
            inspector.InspectLHSMRPartition();
        } break;
        default: {
            LHSMRPartitionInspector<MultiCardPartitionElementProvider> inspector(
                    this, result, same_lhs_validators, lhs);
            inspector.InspectLHSMRPartition();
        } break;
    }
}

auto BatchValidator::GetRemovedAndInterestingness(lattice::ValidationInfo const& info,
                                                  std::vector<model::Index> const& indices)
        -> RemovedAndInterestingnessCCVIds {
    MdLhs const& lhs = info.messenger->GetLhs();

    std::vector<ColumnClassifierValueId> interestingness_ccv_ids;
    auto set_interestingness_ccv_ids = [&](std::vector<ColumnClassifierValueId>& removed_ccv_ids) {
        std::ranges::for_each(removed_ccv_ids, [](ColumnClassifierValueId& ccv_id) { --ccv_id; });
        interestingness_ccv_ids = lattice_->GetInterestingnessCCVIds(lhs, indices, removed_ccv_ids);
        std::ranges::for_each(removed_ccv_ids, [](ColumnClassifierValueId& ccv_id) { ++ccv_id; });
    };
    std::vector<ColumnClassifierValueId> removed_ccv_ids =
            info.messenger->GetRhs().DisableAndDo(indices, set_interestingness_ccv_ids);
    return {std::move(removed_ccv_ids), std::move(interestingness_ccv_ids)};
}

void BatchValidator::FillValidators(
        SameLhsValidators& same_lhs_validators, AllRhsRecommendations& recommendations,
        std::vector<Index> const& pending_rhs_indices,
        RemovedAndInterestingnessCCVIds const& removed_and_interestingness) const {
    {
        std::size_t const rhss_number = pending_rhs_indices.size();
        same_lhs_validators.reserve(rhss_number);
        recommendations.reserve(rhss_number);
    }

    CompressedRecords const& right_records = GetRightCompressor().GetRecords();
    for (auto const& [removed_ccv_ids, interestingness_ccv_ids] = removed_and_interestingness;
         auto [column_match_index, old_ccv_id, interestingness_ccv_id] :
         utility::Zip(pending_rhs_indices, removed_ccv_ids, interestingness_ccv_ids)) {
        OneRhsRecommendations& last_recs = recommendations.emplace_back();
        auto const& [sim_info, left_column_index, right_column_index] =
                (*column_matches_info_)[column_match_index];
        MdElement rhs{column_match_index, old_ccv_id};
        same_lhs_validators.emplace_back(
                rhs, last_recs, GetLeftTableColumnValueNum(column_match_index),
                interestingness_ccv_id, right_records, sim_info.similarity_matrix,
                left_column_index, right_column_index);
    }
}

void BatchValidator::CreateValidators(lattice::ValidationInfo const& info,
                                      SameLhsValidators& same_lhs_validators,
                                      AllRhsRecommendations& recommendations) {
    boost::dynamic_bitset<> const& pending_rhs_indices_bitset = info.rhs_indices_to_validate;
    if (pending_rhs_indices_bitset.none()) return;

    // NOTE: converting to indices because the index list is iterated through many times in
    // MdLattice::GetInterestingnessCCVIds, and it is faster to allocate and iterate through a
    // vector than a bitset.
    std::vector<Index> const pending_rhs_indices =
            util::BitsetToIndices<Index>(pending_rhs_indices_bitset);

    RemovedAndInterestingnessCCVIds removed_and_interestingness =
            GetRemovedAndInterestingness(info, pending_rhs_indices);

    FillValidators(same_lhs_validators, recommendations, pending_rhs_indices,
                   removed_and_interestingness);
}

void BatchValidator::ValidateEmptyLhs(Result& result,
                                      boost::dynamic_bitset<> const& rhs_indices_to_validate,
                                      lattice::Rhs const& lattice_rhs) const {
    util::ForEachIndex(rhs_indices_to_validate, [&](auto index) {
        ColumnClassifierValueId const old_cc_value_id = lattice_rhs[index];
        if (old_cc_value_id == kLowestCCValueId) [[likely]]
            return;
        result.invalidated_rhss.PushBack({index, old_cc_value_id}, kLowestCCValueId);
    });
    result.lhs_is_unsupported = !Supported(GetTotalPairsNum());
}

void BatchValidator::RemoveTrivialForCardinality1Lhs(
        model::Index const lhs_index, Result& result,
        boost::dynamic_bitset<>& rhs_indices_to_validate, lattice::Rhs const& lattice_rhs) {
    // If LHS has cardinality 1 and the column classifier uses a natural decision boundary, then it
    // follows that the RHS column classifier decision boundary must be equal to it, giving us a
    // trivial dependency, no need to go through the full validation process for it.
    // NOTE: this assumes that all LHS boundaries are natural, which may not be true for a more
    // general case.
    // NOTE: Never true when disjointedness pruning is on.
    if (rhs_indices_to_validate.test_set(lhs_index, false)) {
        // Zero CCV ID RHSs are treated as removed and their validation should not be requested.
        DESBORDANTE_ASSUME(lattice_rhs[lhs_index] != kLowestCCValueId);
        result.invalidated_rhss.PushBack({lhs_index, lattice_rhs[lhs_index]}, kLowestCCValueId);
    }
}

template <bool PerformValidation>
decltype(auto) BatchValidator::GetRhsValidatorsContainer() {
    if constexpr (PerformValidation) {
        return SameLhsValidators{};
    } else {
        return current_validator_batch_.emplace_back();
    }
}

template <bool PerformValidation>
void BatchValidator::CreateResult(lattice::ValidationInfo& info) {
    Result& result = results_.emplace_back();

    // Store locally if validation is performed, store in current_validator_batch_ if postponed.
    auto&& rhs_validators = GetRhsValidatorsContainer<PerformValidation>();

    boost::dynamic_bitset<>& rhs_indices_to_validate = info.rhs_indices_to_validate;
    lattice::Rhs& lattice_rhs = info.messenger->GetRhs();

    switch (MdLhs const& lhs = info.messenger->GetLhs(); lhs.Cardinality()) {
        [[unlikely]] case 0:
            // Very cheap, converting to indices is not needed, so might as well validate
            // immediately.
            ValidateEmptyLhs(result, rhs_indices_to_validate, lattice_rhs);
            break;
        case 1:
            RemoveTrivialForCardinality1Lhs(lhs.begin()->offset, result, rhs_indices_to_validate,
                                            lattice_rhs);
            [[fallthrough]];
        default:
            CreateValidators(info, rhs_validators, result.all_rhs_recommendations);
            if constexpr (PerformValidation) Validate(info, result, rhs_validators);
            break;
    }
}

template <bool PerformValidation>
void BatchValidator::CreateResults(std::vector<lattice::ValidationInfo>& all_validation_info) {
    std::size_t const validations_number = all_validation_info.size();
    results_.clear();
    utility::ReserveMore(results_, validations_number);

    if constexpr (!PerformValidation) {
        current_validator_batch_.clear();
        utility::ReserveMore(current_validator_batch_, validations_number);
    }

    for (lattice::ValidationInfo& info : all_validation_info) {
        CreateResult<PerformValidation>(info);
    }
}

auto BatchValidator::ValidateBatch(std::vector<lattice::ValidationInfo>& minimal_lhs_mds)
        -> std::vector<Result> const& {
    if (pool_ == nullptr) {
        CreateResults<true>(minimal_lhs_mds);
    } else {
        CreateResults<false>(minimal_lhs_mds);
        auto validate_at_index = [&](Index i) {
            Validate(minimal_lhs_mds[i], results_[i], current_validator_batch_[i]);
        };
        pool_->ExecIndex(validate_at_index, minimal_lhs_mds.size());
    }
    return results_;
}

}  // namespace algos::hymd
