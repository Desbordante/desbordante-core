#include "algorithms/mde/hymde/cover_calculation/batch_validator.h"

#include <span>
#include <vector>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/utility/reserve_more.h"
#include "algorithms/mde/hymde/utility/trivial_array.h"
#include "model/index.h"
#include "util/erase_if_replace.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::cover_calculation {
using CompressedRecords = record_match_indexes::PartitionIndex::RecordClustersMapping;
using lattice::MdeLhs;
using model::Index;
using record_match_indexes::UpperSet;
using IndexVector = std::vector<Index>;
using RecIdVec = std::vector<RecordIdentifier>;
using CompressedRecord = record_match_indexes::PartitionIndex::Clusters;
using PliCluster = record_match_indexes::PartitionIndex::RecordCluster;

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

UpperSet const* BatchValidator::GetSimilarRecords(Index const record_match_index,
                                                  PartitionValueId const value_id,
                                                  model::Index const lhs_rcv_id) const {
    record_match_indexes::UpperSetIndex const& similarity_index =
            (*record_match_indexes_)[record_match_index].upper_set_index;
    record_match_indexes::ValueUpperSetMapping const& mapping = similarity_index[value_id];
    // Upper sets for zero RCV IDs are the entire right table and should be neither stored nor
    // requested.
    DESBORDANTE_ASSUME(lhs_rcv_id != kLowestRCValueId);
    return mapping.GetUpperSet(lhs_rcv_id);
}

template <typename LHSMRPartitionElementProvider>
class BatchValidator::LHSMRPartitionInspector {
    using PartitionElement = LHSMRPartitionElementProvider::PartitionElement;

    BatchValidator const* const validator_;
    CompressedRecords const& left_records_ =
            validator_->GetLeftPartitionIndex().GetClustersMapping();
    Result& result_;
    SameLhsValidators& rhs_validators_;
    LHSMRPartitionElementProvider lhsmr_partition_element_provider_;

    [[nodiscard]] MdValidationStatus LowerRCVIDsAndCollectPairs(
            RhsValidator& rhs_validator, PliCluster const& cluster,
            UpperSet const& similar_records) const;
    [[nodiscard]] MdValidationStatus LowerRCVIDsAndCollectPairs(
            RhsValidator& rhs_validator, RecordCluster const& matched_records,
            RecIdVec const& similar_records) const;

    bool LhsIsSupported() {
        return validator_->Supported(result_.support);
    }

    void MakeOutOfClustersResult() {
        if (!LhsIsSupported()) {
            return;
        }

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

            result_.support += partition_element_opt->GetPairsNumber();

            bool const finish_early = element_action(*partition_element_opt);
            if (finish_early) return;
        }
        finish_out_of_pairs();
    }

    void MakeAllInvalidatedResult() {
        // All RHSs that were invalidated should have had their validators deleted.
        DESBORDANTE_ASSUME(rhs_validators_.empty());
        auto check_support = [&](auto&&...) { return LhsIsSupported(); };
        if (check_support()) return;

        ForEachPartitionElement(check_support, []() {});
    }

    // Lower RCV IDs, collect prospective pairs, remove invalidated (i.e. with RCV ID 0) RHSs.
    bool InspectPartitionElement(PartitionElement element) {
        util::EraseIfReplace(rhs_validators_, [&](RhsValidator& rhs_validator) {
            auto const& [lhs_records, matched_rhs_records] = element;
            MdValidationStatus const status =
                    LowerRCVIDsAndCollectPairs(rhs_validator, lhs_records, matched_rhs_records);
            if (status == MdValidationStatus::kInvalidated) {
                result_.invalidated_rhss.PushBack(rhs_validator.GetOldRhs(), kLowestRCValueId);
                return true;
            }
            return false;
        });
        return rhs_validators_.empty();
    }

public:
    LHSMRPartitionInspector(BatchValidator const* validator, Result& result,
                            SameLhsValidators& rhs_validators, MdeLhs const& lhs)
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
auto BatchValidator::RhsValidator::LowerRCVIDAndCollectRecommendations(
        RecordCluster const& lhs_records, Collection const& matched_rhs_records)
        -> MdValidationStatus {
    // Invalidated are removed (1), empty LHSMR partition "elements" should have been skipped (2,
    // 3), non-minimal are considered invalidated (4).
    DESBORDANTE_ASSUME(current_rcv_id_ != kLowestRCValueId);        // 1
    DESBORDANTE_ASSUME(!matched_rhs_records.empty());               // 2
    DESBORDANTE_ASSUME(!lhs_records.empty());                       // 3
    DESBORDANTE_ASSUME(interestingness_rcv_id_ < current_rcv_id_);  // 4

    LeftValueGroupedLhsRecords grouped_lhs_records = GroupLhsRecords(lhs_records);

    for (auto const& [left_column_value_id, same_left_value_records] : grouped_lhs_records) {
        DESBORDANTE_ASSUME(matched_rhs_records.begin() != matched_rhs_records.end());
        for (RecordIdentifier rhs_record_id : matched_rhs_records) {
            CompressedRecord const& right_record = (*right_clusters_)[rhs_record_id];

            bool const md_invalidated = LowerRCVID(left_column_value_id, right_record);
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
        LowerRCVIDsAndCollectPairs(RhsValidator& rhs_validator, RecordCluster const& lhs_records,
                                   RecIdVec const& matched_rhs_records) const
        -> MdValidationStatus {
    return rhs_validator.LowerRCVIDAndCollectRecommendations(lhs_records, matched_rhs_records);
}

template <typename LHSMRPartitionElementProvider>
auto BatchValidator::LHSMRPartitionInspector<LHSMRPartitionElementProvider>::
        LowerRCVIDsAndCollectPairs(RhsValidator& rhs_validator, PliCluster const& cluster,
                                   UpperSet const& matched_rhs_records) const
        -> MdValidationStatus {
    // See LowerRCVIDAndCollectRecommendations comment
    DESBORDANTE_ASSUME(!matched_rhs_records.empty());

    RecordCluster cluster_records = util::GetPreallocatedVector<RecPtr>(cluster.size());

    DESBORDANTE_ASSUME(!cluster.empty());
    for (RecordIdentifier left_record_id : cluster) {
        cluster_records.push_back(&left_records_[left_record_id]);
    }
    return rhs_validator.LowerRCVIDAndCollectRecommendations(cluster_records, matched_rhs_records);
}

// Each left table column's PLI already stores the desired partition of the left table. The sets of
// records that are matched by each partition key's value are already stored in similarity indexes.
class BatchValidator::OneCardPartitionElementProvider {
    BatchValidator const* const validator_;
    PartitionValueId current_value_id_ = PartitionValueId(-1);
    Index const lhs_record_match_index_;
    RecordClassifierValueId const lhs_rcv_id_;
    record_match_indexes::PartitionIndex::PositionListIndex const& clusters_ =
            validator_->GetLeftPartitionIndex().GetPli(lhs_record_match_index_);
    std::size_t const clusters_size_ = clusters_.size();

public:
    using PartitionElement = LHSMRPartitionElement<PliCluster, UpperSet>;

    OneCardPartitionElementProvider(BatchValidator const* validator, MdeLhs const& lhs)
        : validator_(validator),
          lhs_record_match_index_(lhs.begin()->offset),
          lhs_rcv_id_(lhs.begin()->rcv_id) {}

    std::optional<PartitionElement> TryGetNextLHSMRPartitionElement() {
        while (++current_value_id_ != clusters_size_) {
            UpperSet const* const similar_records_ptr = validator_->GetSimilarRecords(
                    lhs_record_match_index_, current_value_id_, lhs_rcv_id_);
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

    // Find RHS records that have a value similar with respect to `rcv_id` to
    // `sltvpe_partition_value[i]` according to the measure of the record match at index
    // `record_match_index`.
    struct RhsRecordsMatchingCriterion {
        Index record_match_index;
        RecordClassifierValueId rcv_id;
    };

    using RhsRecordsMatchingCriteria = std::vector<RhsRecordsMatchingCriterion>;

    class Initializer {
        BatchValidator const* validator_;

        MdeLhs::iterator lhs_iter_;
        MdeLhs::iterator const lhs_end_;

        RhsRecordsMatchingCriterion first_criterion_;
        RhsRecordsMatchingCriteria rhs_records_matching_criteria_;

        Index cur_col_match_index_ = lhs_iter_->offset;

        void AddFirstRhsRecordsMatchingCriterion() {
            // LHS has cardinality greater than 1, so is not empty.
            DESBORDANTE_ASSUME(lhs_iter_ != lhs_end_);
            first_criterion_ = {cur_col_match_index_, lhs_iter_->rcv_id};
            ++cur_col_match_index_;
            ++lhs_iter_;
        }

        void AddOtherRhsRecordsMatchingCriteria() {
            // LHS has cardinality greater than 1, so it has at least one more element.
            DESBORDANTE_ASSUME(lhs_iter_ != lhs_end_);
            for (auto const& [next_node_offset, rcv_id] : std::span{lhs_iter_, lhs_end_}) {
                cur_col_match_index_ += next_node_offset;

                rhs_records_matching_criteria_.push_back({cur_col_match_index_, rcv_id});
                ++cur_col_match_index_;
            }
        }

    public:
        Initializer(BatchValidator const* validator, MdeLhs const& lhs)
            : validator_(validator), lhs_iter_(lhs.begin()), lhs_end_(lhs.end()) {
            std::size_t const cardinality = lhs.Cardinality();
            // This class is adapted for this case and should only be created if this holds.
            DESBORDANTE_ASSUME(cardinality > 1);
            rhs_records_matching_criteria_.reserve(cardinality - 1);

            AddFirstRhsRecordsMatchingCriterion();
            AddOtherRhsRecordsMatchingCriteria();
        }

        BatchValidator const* GetValidator() const noexcept {
            return validator_;
        }

        Index GetSLTVPEPartitionRecordMatchIndex() const noexcept {
            return first_criterion_.record_match_index;
        }

        RhsRecordsMatchingCriterion const& GetFirstCriterion() const noexcept {
            return first_criterion_;
        }

        RhsRecordsMatchingCriteria& GetRhsRecordsMatchingCriteria() noexcept {
            return rhs_records_matching_criteria_;
        }
    };

    using ValueIdArray = utility::TrivialArray<PartitionValueId>;
    using GroupMap = std::unordered_map<ValueIdArray, RecordCluster, ValueIdArray::Hasher,
                                        ValueIdArray::ArrEqual>;

    BatchValidator const* const validator_;

    std::vector<PliCluster> const& sltv_partition_;
    std::size_t const sltv_partition_size_ = sltv_partition_.size();

    RhsRecordsMatchingCriterion const first_criterion_;
    RhsRecordsMatchingCriteria const rhs_records_matching_criteria_;

    std::size_t const sltvpe_partition_value_size_ = rhs_records_matching_criteria_.size();
    ValueIdArray partition_value_scratch_{sltvpe_partition_value_size_};

    GroupMap sltvpe_partition_{0, ValueIdArray::Hasher(sltvpe_partition_value_size_),
                               ValueIdArray::ArrEqual(sltvpe_partition_value_size_)};
    GroupMap::iterator sltvpe_partition_iter_ = sltvpe_partition_.begin();

    // TODO: permute record matches for validation so that this partition is the one with the
    // largest amount of clusters or with the smallest maximum cluster size.
    PartitionValueId sltv_partition_value_ = 0;

    std::vector<UpperSet const*> matched_rhs_rec_sets_scratch_;
    CompressedRecords const& left_records_ =
            validator_->GetLeftPartitionIndex().GetClustersMapping();
    RecIdVec rhs_set_intersection_;

    MultiCardPartitionElementProvider(Initializer init_info)
        : validator_(init_info.GetValidator()),
          sltv_partition_(validator_->GetLeftPartitionIndex().GetPli(
                  init_info.GetSLTVPEPartitionRecordMatchIndex())),
          first_criterion_(init_info.GetFirstCriterion()),
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
            CompressedRecord const& record = left_records_[record_id];
            for (auto const& [record_match_index, rcv_id] : rhs_records_matching_criteria_) {
                inserter.PushBack(record[record_match_index]);
            }
            sltvpe_partition_[partition_value_scratch_.GetSpan(sltvpe_partition_value_size_)]
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

    bool AddUpperSet(RhsRecordsMatchingCriterion const& criterion, PartitionValueId pvid) {
        auto const& [record_match_index, rcv_id] = criterion;
        UpperSet const* similar_records_ptr =
                validator_->GetSimilarRecords(record_match_index, pvid, rcv_id);
        if (similar_records_ptr == nullptr) return true;
        // Empty sets shouldn't be stored in similarity indexes.
        DESBORDANTE_ASSUME(!similar_records_ptr->empty());
        matched_rhs_rec_sets_scratch_.push_back(similar_records_ptr);
        return false;
    }

    bool FindMatchedRhsRecordSets(ValueIdArray const& sltvpe_partition_value) {
        matched_rhs_rec_sets_scratch_.clear();

        if (AddUpperSet(first_criterion_, sltv_partition_value_)) return true;

        auto const span = sltvpe_partition_value.GetSpan(rhs_records_matching_criteria_.size());
        for (auto const [criterion, value_id] :
             utility::Zip(rhs_records_matching_criteria_, span)) {
            if (AddUpperSet(criterion, value_id)) return true;
        }
        return false;
    }

    void SortMatchedRhsRecordSetsBySize() {
        // Allows to loop through the smallest possible number of elements due to the smallest set
        // being placed first. As for the other sets, we can say that the smaller a set is, the more
        // likely it is that an element won't be contained in it, making us stop an iteration for an
        // element sooner on average when they are sorted by size.
        auto size_cmp = [](UpperSet const* p1, UpperSet const* p2) {
            return p1->size() < p2->size();
        };
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

        UpperSet const& first = *matched_rhs_rec_sets_scratch_.front();
        std::span other_record_set_ptrs{matched_rhs_rec_sets_scratch_.begin() + 1,
                                        matched_rhs_rec_sets_scratch_.end()};
        // Empty sets have been excluded prior to the call.
        DESBORDANTE_ASSUME(first.begin() != first.end());
        for (RecordIdentifier rec : first) {
            auto contains_record = [rec](UpperSet const* set_ptr) {
                return set_ptr->contains(rec);
            };
            // At least two column matches are considered.
            DESBORDANTE_ASSUME(other_record_set_ptrs.begin() != other_record_set_ptrs.end());
            if (std::ranges::all_of(other_record_set_ptrs, contains_record)) {
                rhs_set_intersection_.push_back(rec);
            }
        }
    }

public:
    using PartitionElement = LHSMRPartitionElement<RecordCluster, RecIdVec>;

    MultiCardPartitionElementProvider(BatchValidator const* validator, MdeLhs const& lhs)
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

void BatchValidator::Validate(ValidationSelection& info, Result& result,
                              SameLhsValidators& same_lhs_validators) const {
    switch (MdeLhs const& lhs = info.updater->GetLhs(); lhs.Cardinality()) {
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

auto BatchValidator::GetRemovedAndInterestingness(ValidationSelection const& info,
                                                  std::vector<model::Index> const& indices)
        -> RemovedAndInterestingnessRCVIds {
    MdeLhs const& lhs = info.updater->GetLhs();

    std::vector<RecordClassifierValueId> interestingness_rcv_ids;
    auto set_interestingness_rcv_ids = [&](std::vector<RecordClassifierValueId>& removed_rcv_ids) {
        std::ranges::for_each(removed_rcv_ids, [](RecordClassifierValueId& rcv_id) { --rcv_id; });
        interestingness_rcv_ids = lattice_->GetInterestingnessRCVIds(lhs, indices, removed_rcv_ids);
        std::ranges::for_each(removed_rcv_ids, [](RecordClassifierValueId& rcv_id) { ++rcv_id; });
    };
    std::vector<RecordClassifierValueId> removed_rcv_ids =
            info.updater->GetRhs().DisableAndDo(indices, set_interestingness_rcv_ids);
    return {std::move(removed_rcv_ids), std::move(interestingness_rcv_ids)};
}

void BatchValidator::FillValidators(
        SameLhsValidators& same_lhs_validators, AllRhsRecommendations& recommendations,
        std::vector<Index> const& pending_rhs_indices,
        RemovedAndInterestingnessRCVIds const& removed_and_interestingness) const {
    {
        std::size_t const rhss_number = pending_rhs_indices.size();
        same_lhs_validators.reserve(rhss_number);
        recommendations.reserve(rhss_number);
    }

    CompressedRecords const& right_records = GetRightPartitionIndex().GetClustersMapping();
    for (auto const& [removed_rcv_ids, interestingness_rcv_ids] = removed_and_interestingness;
         auto [record_match_index, old_rcv_id, interestingness_rcv_id] :
         utility::Zip(pending_rhs_indices, removed_rcv_ids, interestingness_rcv_ids)) {
        OneRhsRecommendations& last_recs = recommendations.emplace_back();
        auto const& [value_matrix, upper_set_index] = (*record_match_indexes_)[record_match_index];
        MdeElement rhs{record_match_index, old_rcv_id};
        same_lhs_validators.emplace_back(
                rhs, last_recs, GetLeftTablePliValueNum(record_match_index), interestingness_rcv_id,
                right_records, value_matrix, record_match_index);
    }
}

void BatchValidator::CreateValidators(ValidationSelection const& info,
                                      SameLhsValidators& same_lhs_validators,
                                      AllRhsRecommendations& recommendations) {
    boost::dynamic_bitset<> const& pending_rhs_indices_bitset = info.rhs_indices_to_validate;
    if (pending_rhs_indices_bitset.none()) return;

    // NOTE: converting to indices because the index list is iterated through many times in
    // MdLattice::GetInterestingnessRCVIds, and it is faster to allocate and iterate through a
    // vector than a bitset.
    std::vector<Index> const pending_rhs_indices =
            util::BitsetToIndices<Index>(pending_rhs_indices_bitset);

    RemovedAndInterestingnessRCVIds removed_and_interestingness =
            GetRemovedAndInterestingness(info, pending_rhs_indices);

    FillValidators(same_lhs_validators, recommendations, pending_rhs_indices,
                   removed_and_interestingness);
}

void BatchValidator::ValidateEmptyLhs(Result& result,
                                      boost::dynamic_bitset<> const& rhs_indices_to_validate,
                                      lattice::Rhs const& lattice_rhs) const {
    util::ForEachIndex(rhs_indices_to_validate, [&](auto index) {
        RecordClassifierValueId const old_cc_value_id = lattice_rhs[index];
        if (old_cc_value_id == kLowestRCValueId) [[likely]]
            return;
        result.invalidated_rhss.PushBack({index, old_cc_value_id}, kLowestRCValueId);
    });
    result.support = GetTotalPairsNum();
}

void BatchValidator::RemoveTrivialForCardinality1Lhs(
        model::Index const lhs_index, Result& result,
        boost::dynamic_bitset<>& rhs_indices_to_validate, lattice::Rhs const& lattice_rhs) {
    // If LHS has cardinality 1 and the column classifier uses a natural decision boundary, then it
    // follows that the RHS column classifier decision boundary must be equal to it, giving us a
    // trivial dependency, no need to go through the full validation process for it.
    // NOTE: Never true when disjointedness pruning is on.
    if (rhs_indices_to_validate.test_set(lhs_index, false)) {
        // Zero RCV ID RHSs are treated as removed and their validation should not be requested.
        DESBORDANTE_ASSUME(lattice_rhs[lhs_index] != kLowestRCValueId);
        result.invalidated_rhss.PushBack({lhs_index, lattice_rhs[lhs_index]}, kLowestRCValueId);
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
void BatchValidator::CreateResult(ValidationSelection& selections) {
    Result& result = results_.emplace_back();

    // Store locally if validation is performed, store in current_validator_batch_ if postponed.
    auto&& rhs_validators = GetRhsValidatorsContainer<PerformValidation>();

    boost::dynamic_bitset<>& rhs_indices_to_validate = selections.rhs_indices_to_validate;
    lattice::Rhs& lattice_rhs = selections.updater->GetRhs();

    switch (MdeLhs const& lhs = selections.updater->GetLhs(); lhs.Cardinality()) {
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
            CreateValidators(selections, rhs_validators, result.all_rhs_recommendations);
            if constexpr (PerformValidation) Validate(selections, result, rhs_validators);
            break;
    }
}

template <bool PerformValidation>
void BatchValidator::CreateResults(std::vector<ValidationSelection>& selections) {
    std::size_t const validations_number = selections.size();
    results_.clear();
    utility::ReserveMore(results_, validations_number);

    if constexpr (!PerformValidation) {
        current_validator_batch_.clear();
        utility::ReserveMore(current_validator_batch_, validations_number);
    }

    for (ValidationSelection& info : selections) {
        CreateResult<PerformValidation>(info);
    }
}

auto BatchValidator::ValidateBatch(std::vector<ValidationSelection>& selections)
        -> std::vector<Result> const& {
    if (pool_ == nullptr) {
        CreateResults<true>(selections); /*lattice::MdeLhs const& lattice_lhs, lattice::Rhs const&
                  rhs, ValidationSelection& selection, InvalidatedRhss const& invalidated,
                  std::size_t support*/
        ;
    } else {
        CreateResults<false>(selections);
        auto validate_at_index = [&](Index i) {
            Validate(selections[i], results_[i], current_validator_batch_[i]);
        };
        pool_->ExecIndex(validate_at_index, selections.size());
    }
    assert(selections.size() == results_.size());
    for (auto const& [selection, result] : utility::Zip(selections, results_)) {
        if (!Supported(result.support)) continue;
        validated_adder_.AddMDEs(selection, result.invalidated_rhss, result.support);
    }
    return results_;
}
}  // namespace algos::hymde::cover_calculation
