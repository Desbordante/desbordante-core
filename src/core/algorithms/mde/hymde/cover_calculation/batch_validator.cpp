#include "algorithms/mde/hymde/cover_calculation/batch_validator.h"

#include <cassert>
#include <numeric>
#include <span>
#include <vector>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/utility/not_empty.h"
#include "algorithms/mde/hymde/utility/reserve_more.h"
#include "algorithms/mde/hymde/utility/trivial_array.h"
#include "model/index.h"
#include "util/erase_if_replace.h"
#include "util/get_preallocated_vector.h"
#include "util/py_tuple_hash.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::cover_calculation {
using TablePartitionValueIdMaps = record_match_indexes::PartitionIndex::TablePartitionValueIdMaps;
using lattice::MdeLhs;
using model::Index;
using record_match_indexes::PartValueSet;
using IndexVector = std::vector<Index>;
using PartitionValueIdMap = record_match_indexes::PartitionIndex::PartitionValueIdMap;
using PliCluster = record_match_indexes::PartitionIndex::PliCluster;
using PositionListIndex = record_match_indexes::PartitionIndex::PositionListIndex;

static constexpr RecordIdentifier kNoMoreRecords = -1;

// There are several partitions that are going to be mentioned here.
// Firstly, a partition of the left table based on equality of values of partitioning functions
// (`LTPVsPartition` in code, from "left table partition values partition"). Note that we create a
// partition for each partitioning function in the preprocessing stage (`SLTPVPartition` here, from
// "single left table partition value partition"). Thus, for any partition where the key has
// multiple partitioning functions we can obtain the sets of the partition by partitioning each
// element of one of the function's partition (also referred to as "base partition") "properly" only
// on the other functions' results (`BasePEPartition`, "base partition element partition"). Any
// element of BasePEPartition is an element of the corresponding LTPVsPartition.

// Finally, partition of the set of record pairs that are matched by an LHS (`LHSMRPartition`, "LHS
// matched records partition"). Its sets can be obtained by inspecting each set of the
// LTPVsPartition with the partition key being a set of the left table partitioning functions of the
// LHS's record classifiers (if \psi is the set of record classifiers of the LHS, then the set is
// {T | \exists V,F,ord,value ((T, V, F, ord), value) \in \psi}). Along with the partition values,
// each left table's record in this set also shares the set of right table's records that are
// matched by the LHS to it with the others, if any. The Cartesian products of the described sets
// are the elements of LHSMRPartition.

template <typename Filter>
class RightTableIterator {
    PliCluster::const_iterator cur_rt_record_iter_;
    PliCluster::const_iterator end_rt_record_iter_;
    [[no_unique_address]] Filter filter_;
    std::span<PartitionValueId const>::iterator rt_pvalue_id_iter_;
    std::span<PartitionValueId const>::iterator end_rt_pvalue_id_iter_;
    PositionListIndex const& iterated_rt_pli_;

    void SetClusterIters() {
        PliCluster const& current_rt_cluster =
                utility::NotEmpty(iterated_rt_pli_[*rt_pvalue_id_iter_]);
        cur_rt_record_iter_ = current_rt_cluster.begin();
        end_rt_record_iter_ = current_rt_cluster.end();
    }

    bool NextCluster() {
        if (++rt_pvalue_id_iter_ == end_rt_pvalue_id_iter_) return false;
        SetClusterIters();
        return true;
    }

    template <bool IterGuaranteedValid = false>
    RecordIdentifier NextValidRecordInternal() {
        if constexpr (IterGuaranteedValid) goto cur_is_valid;
        while (true) {
            if (cur_rt_record_iter_ == end_rt_record_iter_) {
                if (!NextCluster()) return kNoMoreRecords;
            }

        cur_is_valid:
            RecordIdentifier potential_record_id = *cur_rt_record_iter_++;
            DESBORDANTE_ASSUME(potential_record_id != kNoMoreRecords);
            if (filter_(potential_record_id)) return potential_record_id;
        }
    }

public:
    RightTableIterator(Filter filter, PositionListIndex const& iterated_rt_pli_,
                       std::span<PartitionValueId const> rt_pvalue_ids,
                       RecordIdentifier* first_rt_record_id)
        : filter_(std::move(filter)),
          rt_pvalue_id_iter_(rt_pvalue_ids.begin()),
          end_rt_pvalue_id_iter_(rt_pvalue_ids.end()),
          iterated_rt_pli_(iterated_rt_pli_) {
        SetClusterIters();
        *first_rt_record_id = NextValidRecordInternal<true>();
    }

    RecordIdentifier NextValidRecord() {
        return NextValidRecordInternal();
    }
};

// This class represents an element of the LHSMRPartition, containing information to inspect
// elements of the Cartesian product.
template <typename RightTablePVIDMapIter>
struct LHSMRPartitionElementInfo {
    std::vector<PartitionValueIdMap const*> const& lt_pvid_maps;
    RecordIdentifier first_rt_record_id;
    RightTablePVIDMapIter rt_pvid_map_iter;
};

template <typename LHSMRPartitionElementProvider>
class BatchValidator::LHSMRPartitionInspector {
    using PartitionElement = LHSMRPartitionElementProvider::PartitionElement;

    BatchValidator const* const validator_;
    Result& result_;
    SameLhsValidators& rhs_validators_;
    TablePartitionValueIdMaps const& rt_pvid_maps_ =
            validator_->GetRightTablePartitionIndex().GetPartitionValueIdMaps();
    LHSMRPartitionElementProvider lhsmr_partition_element_provider_;

    bool LhsIsSupported() {
        return validator_->Supported(result_.support);
    }

    // Lower RCV IDs, add recommendation, remove invalidated RHSs.
    bool InspectRhsRecord(PartitionValueIdMap const& rt_record_pvid_map) {
        util::EraseIfReplace(rhs_validators_, [&](RhsValidator& rhs_validator) {
            if (rhs_validator.CheckRecord(rt_record_pvid_map)) {
                result_.invalidated_rhss.PushBack(rhs_validator.GetOldRhs(), kLowestRCValueId);
                return true;
            }
            return false;
        });
        if (rhs_validators_.empty()) {
            MakeAllInvalidatedResult();
            return true;
        }
        return false;
    }

    void ForEachPartitionElement(auto&& on_new_partition_element, auto&& check_record,
                                 auto&& finish_out_of_pairs) {
        while (std::optional<PartitionElement> partition_element_opt =
                       lhsmr_partition_element_provider_.TryGetNextLHSMRPartitionElement()) {
            auto& [lt_pvid_maps, first_rt_record_id, next_rt_pvid_map_getter] =
                    *partition_element_opt;
            std::size_t const lt_records_count = lt_pvid_maps.size();
            on_new_partition_element(lt_pvid_maps);

            DESBORDANTE_ASSUME(first_rt_record_id != kNoMoreRecords);
            RecordIdentifier record_id = first_rt_record_id;
            do {
                result_.support += lt_records_count;
                assert(record_id < rt_pvid_maps_.size());
                if (check_record(rt_pvid_maps_[record_id])) return;
            } while ((record_id = next_rt_pvid_map_getter.NextValidRecord()) != kNoMoreRecords);
        }
        finish_out_of_pairs();
    }

    void MakeOutOfClustersResult() {
        if (!LhsIsSupported()) return;

        // If we ran out of clusters, then it means some RHSs were not invalidated and their
        // corresponding validators were not removed.
        for (RhsValidator const& rhs_validator : utility::NotEmpty(rhs_validators_)) {
            rhs_validator.AddIfInvalid(result_.invalidated_rhss);
        }
    }

    void MakeAllInvalidatedResult() {
        // All RHSs that were invalidated should have had their validators deleted.
        assert(rhs_validators_.empty());
        if (LhsIsSupported()) return;

        ForEachPartitionElement([](auto&&) {}, [this](auto&&) { return LhsIsSupported(); },
                                []() {});
    }

public:
    LHSMRPartitionInspector(BatchValidator const* validator, Result& result,
                            SameLhsValidators& rhs_validators, MdeLhs const& lhs)
        : validator_(validator),
          result_(result),
          rhs_validators_(rhs_validators),
          lhsmr_partition_element_provider_(validator, lhs) {}

    void InspectLHSMRPartition() {
        ForEachPartitionElement(
                [this](LTPVsPEPVIdMaps const& ltpvspe_pvid_maps) {
                    for (RhsValidator& rhs_validator : utility::NotEmpty(rhs_validators_)) {
                        rhs_validator.SetCurrentLTPVsPElement(ltpvspe_pvid_maps);
                    }
                },
                [this](PartitionValueIdMap const& rhs_record_pvid_map) {
                    return InspectRhsRecord(rhs_record_pvid_map);
                },
                [this]() { MakeOutOfClustersResult(); });
    }
};

PartValueSet BatchValidator::GetMatchedValuesSet(Index record_match_index,
                                                 PartitionValueId lt_pvalue_id,
                                                 RecordClassifierValueId lhs_rcv_id) const {
    return (*record_match_indexes_)[record_match_index]
            .upper_set_index[lt_pvalue_id]
            .GetMatchedValues(lhs_rcv_id);
}

class BatchValidator::OneCardPartitionElementProvider {
    BatchValidator const* const validator_;
    PartitionValueId current_value_id_ = PartitionValueId(-1);
    Index const lhs_record_match_index_;
    RecordClassifierValueId const lhs_rcv_id_;
    PositionListIndex const& lt_pli_ =
            validator_->GetLeftTablePartitionIndex().GetPli(lhs_record_match_index_);
    std::size_t const lt_pli_size_ = lt_pli_.size();
    PositionListIndex const& rt_pli_ =
            validator_->GetRightTablePartitionIndex().GetPli(lhs_record_match_index_);
    TablePartitionValueIdMaps const& lt_pvid_maps_ =
            validator_->GetLeftTablePartitionIndex().GetPartitionValueIdMaps();
    LTPVsPEPVIdMaps ltpvsp_element_pvid_maps_scratch_;

    void FillLTPVsPEScratch() {
        ltpvsp_element_pvid_maps_scratch_.clear();
        PliCluster const& lt_records = lt_pli_[current_value_id_];
        utility::ReserveMore(ltpvsp_element_pvid_maps_scratch_, lt_records.size());
        for (RecordIdentifier lt_record_id : utility::NotEmpty(lt_records)) {
            ltpvsp_element_pvid_maps_scratch_.push_back(&lt_pvid_maps_[lt_record_id]);
        }
    }

public:
    static constexpr auto kNoFilter = [](RecordIdentifier) { return true; };

    using RTIterator = RightTableIterator<decltype(kNoFilter)>;
    using PartitionElement = LHSMRPartitionElementInfo<RTIterator>;

    OneCardPartitionElementProvider(BatchValidator const* validator, MdeLhs const& lhs)
        : validator_(validator),
          lhs_record_match_index_(lhs.begin()->offset),
          lhs_rcv_id_(lhs.begin()->rcv_id) {}

    std::optional<PartitionElement> TryGetNextLHSMRPartitionElement() {
        while (++current_value_id_ != lt_pli_size_) {
            PartValueSet similar_values = validator_->GetMatchedValuesSet(
                    lhs_record_match_index_, current_value_id_, lhs_rcv_id_);
            if (similar_values.IsEmpty()) continue;

            FillLTPVsPEScratch();

            RecordIdentifier first_rt_record;
            RTIterator iter{kNoFilter, rt_pli_, similar_values.values, &first_rt_record};
            return {{ltpvsp_element_pvid_maps_scratch_, first_rt_record, std::move(iter)}};
        }
        return std::nullopt;
    }
};

// Basic idea: go over every element of the base partition (NOTE: it's easy to pick any one of those
// that are passed as the base partition, which is suspiciously a lot of freedom, is there an
// optimization opportunity here?).
// Find all value combinations for the other partitions in the element. For every [base value,
// others...] value combination find the sets of right table records that are matched by LHS's
// record classifiers to the left table records with this value combination. Find the smallest right
// table record set.
// Iterate through all records in that set. Pick only the right table records that are present in
// all the other sets.
// An equivalent (and actually more fundamental) formulation of "present in all the other sets" is
// "matched by all the other record classifiers to the left table's records".
// This fact can be checked by using value matrices in constant time, so we don't need to store OR
// construct the actual sets. The constant is slightly higher, though, as we have to look at the
// record's partition value ID map ("compressed record" in HyMD), which is additional an array
// access, plus do a comparison. The last one is negligible, though, memory accesses are way slower.

// What are the things the algorithm cares about in a record classifier? A record match is two
// partitioning functions, a function, an order. A decision boundary is the second part of a record
// classifier. Partitioning functions correspond to a partition identifier (which just happens to be
// the same as the record match index in this implementation), a function gives us a value matrix
// and upper set index. Order and decision boundary give record classifier value IDs.

// When we make a partition, we only care about partition identifiers. When using a record
// classifier, however, we need to know which partition value to access, i.e. which array index this
// value is at in the LTPVsPartition value. What if we store a representative record and get its
// values instead? Then we don't have to bother with indices into that smaller array. Unpredictable
// accesses? We have to reorder the value sets to speed things up, they are going to be
// unpredictable no matter what.

// For every new record match arrangement, we need a (left table) partition ID arrangement.
class BatchValidator::MultiCardPartitionElementProvider {
    struct AtomicConstraintInfo {
        Index record_match_index;
        RecordClassifierValueId usindex_rcv_id;
        RecordClassifierValueId vm_rcv_id;
    };

    using AtomicConstraintsInfo = std::vector<AtomicConstraintInfo>;

    class Initializer {
        BatchValidator const* validator_;
        std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps_ =
                *validator_->rcv_id_lr_maps_;

        MdeLhs::iterator lhs_iter_;
        MdeLhs::iterator const lhs_end_;

        Index cur_rec_match_index_ = lhs_iter_->offset;

        // TODO: permute record matches for validation so that this partition is the one with the
        // largest amount of clusters or with the smallest maximum cluster size.
        Index base_partition_index_;
        std::vector<Index> basepe_partition_key_indices_;

        AtomicConstraintsInfo lhs_atomic_constraints_;

        void AddConstraint(RecordClassifierValueId const lhs_rcv_id) {
            lhs_atomic_constraints_.emplace_back(
                    cur_rec_match_index_, lhs_rcv_id,
                    rcv_id_lr_maps_[cur_rec_match_index_].lhs_to_rhs_map[lhs_rcv_id]);
        }

        void AddFirstRhsRecordsMatchingCriterion() {
            base_partition_index_ =
                    cur_rec_match_index_; /*rm_indices[cur_rec_match_index_].lt_partition_id*/

            // LHS has cardinality greater than 1, so is not empty.
            DESBORDANTE_ASSUME(lhs_iter_ != lhs_end_);
            AddConstraint(lhs_iter_->rcv_id);
            ++cur_rec_match_index_;
            ++lhs_iter_;
        }

        void AddOtherRhsRecordsMatchingCriteria() {
            auto remaining_lhs = std::span{lhs_iter_, lhs_end_};
            // LHS has cardinality greater than 1, so it has at least one more element.
            for (auto const& [next_node_offset, lhs_rcv_id] : utility::NotEmpty(remaining_lhs)) {
                cur_rec_match_index_ += next_node_offset;
                basepe_partition_key_indices_.push_back(
                        cur_rec_match_index_); /*rm_indices[cur_rec_match_index_].lt_partition_id*/
                AddConstraint(lhs_rcv_id);
                ++cur_rec_match_index_;
            }
        }

    public:
        Initializer(BatchValidator const* validator, MdeLhs const& lhs)
            : validator_(validator), lhs_iter_(lhs.begin()), lhs_end_(lhs.end()) {
            std::size_t const cardinality = lhs.Cardinality();
            // This class is adapted for this case and should only be created if this holds.
            DESBORDANTE_ASSUME(cardinality > 1);
            basepe_partition_key_indices_.reserve(cardinality - 1);
            lhs_atomic_constraints_.reserve(cardinality);

            AddFirstRhsRecordsMatchingCriterion();
            AddOtherRhsRecordsMatchingCriteria();
            // Sort (and apply std::unique) if not for better locality, currently should be sorted,
            // as these are just record match indices.
            assert(std::ranges::is_sorted(basepe_partition_key_indices_));
        }

        BatchValidator const* GetValidator() const noexcept {
            return validator_;
        }

        Index GetBasePartitionIndex() const noexcept {
            return base_partition_index_;
        }

        std::vector<Index>&& GetKeyIndices() noexcept {
            return std::move(basepe_partition_key_indices_);
        }

        AtomicConstraintsInfo&& GetConstraints() noexcept {
            return std::move(lhs_atomic_constraints_);
        }
    };

    class IndexHasher {
        std::span<Index> indices_;

    public:
        IndexHasher(std::span<Index> indices) : indices_(indices) {}

        std::size_t operator()(PValueIdMapPtr pvid_map_ptr) const noexcept {
            util::PyTupleHash hasher{indices_.size()};
            for (Index index : indices_) {
                hasher.AddValue((*pvid_map_ptr)[index]);
            }
            return hasher.GetResult();
        }
    };

    class IndexComparer {
        std::span<Index> indices_;

    public:
        IndexComparer(std::span<Index> indices) : indices_(indices) {}

        bool operator()(PValueIdMapPtr pvid_map_ptr1, PValueIdMapPtr pvid_map_ptr2) const noexcept {
            auto equal_at = [&](model::Index index) {
                return (*pvid_map_ptr1)[index] == (*pvid_map_ptr2)[index];
            };
            return std::ranges::all_of(indices_, equal_at);
        }
    };

    // Idea: have a record as the key, value as the collection of records that share this record's
    // values in some partitions as identified by partition IDs (i.e. indices). The key is the
    // *representative* record for that record collection.
    using GroupMap =
            std::unordered_map<PValueIdMapPtr, LTPVsPEPVIdMaps, IndexHasher, IndexComparer>;

    // An atomic constraint in an MDE is a condition in the form of l <= F(T(p), V(q)), p and q
    // are records from the left and right tables respectively.
    // In this algorithm all possible values of F(...), l are matched to integers (RCV IDs) and all
    // possible values of T(p) := x and V(q) := q[y] have been precalculated. x is known, ID of V(q)
    // needs to be extracted
    // So the conditions are l <= F(x, q[y]). This is calculating a predicate with q as the free
    // variable. P[q] ~=~ l <= F(x, q[y]). Results of F have been precalculated in a value matrix M.
    // The condition takes the form of P[q] ~=~ l <= M[x][q[y]]. M[x] is already known, let's name
    // that R. The condition is P[q] ~=~ l <= R[q[y]]. In this struct l is `vm_rcv_id_cutoff`,
    // R is `lt_pvalue_vm_row_ptr`, y is `rt_partition_id`.
    struct PartiallyAppliedAtomicConstraint {
        record_match_indexes::ValueMatrixRow const* lt_pvalue_vm_row_ptr;
        // These fields are always copied, can this be avoided? Should it?
        RecordClassifierValueId vm_rcv_id_cutoff;
        // Happens to be the record match index in this implementation.
        Index rt_partition_id;

        // Whether a representative left table record and the passed right table record satisfy this
        // atomic constraint.
        bool IsSatisified(PartitionValueIdMap const& rt_pvid_map) const {
            PartitionValueId const rt_pvalue_id = rt_pvid_map[rt_partition_id];
            auto it = lt_pvalue_vm_row_ptr->find(rt_pvalue_id);
            return it != lt_pvalue_vm_row_ptr->end() && vm_rcv_id_cutoff <= it->second;
        }
    };

    BatchValidator const* const validator_;

    TablePartitionValueIdMaps const& rt_pvid_maps_ =
            validator_->data_partition_index_->GetRight().GetPartitionValueIdMaps();

    Index base_partition_id_;
    std::vector<PliCluster> const& base_partition_ =
            validator_->GetLeftTablePartitionIndex().GetPli(base_partition_id_);
    std::size_t const base_partition_size_ = base_partition_.size();

    std::vector<Index> basepe_partition_key_indices_;

    std::size_t const basepe_partition_value_size_ = basepe_partition_key_indices_.size();

    GroupMap basepe_partition_{0, IndexHasher(basepe_partition_key_indices_),
                               IndexComparer(basepe_partition_key_indices_)};
    GroupMap::iterator basepe_partition_iter_ = basepe_partition_.begin();

    PartitionValueId base_partition_value_id_ = 0;

    AtomicConstraintsInfo lhs_atomic_constraints_;
    std::size_t lhs_atomic_constraints_size_ = lhs_atomic_constraints_.size();

    std::vector<PartValueSet> matched_rt_value_sets_scratch_;
    std::unique_ptr<Index[]> paac_arrangement_scratch_ =
            std::make_unique<Index[]>(lhs_atomic_constraints_size_);
    std::vector<PartiallyAppliedAtomicConstraint> other_paac_scratch_;

    TablePartitionValueIdMaps const& lt_pvid_maps_ =
            validator_->GetLeftTablePartitionIndex().GetPartitionValueIdMaps();

    MultiCardPartitionElementProvider(Initializer init_info)
        : validator_(init_info.GetValidator()),
          base_partition_id_(init_info.GetBasePartitionIndex()),
          basepe_partition_key_indices_(init_info.GetKeyIndices()),
          lhs_atomic_constraints_(std::move(init_info.GetConstraints())) {
        DESBORDANTE_ASSUME(lhs_atomic_constraints_size_ > 1);
        matched_rt_value_sets_scratch_.reserve(lhs_atomic_constraints_size_);
        other_paac_scratch_.reserve(lhs_atomic_constraints_size_ - 1);

        // Tables are guaranteed to not be empty, so a partition can't be empty either.
        DESBORDANTE_ASSUME(base_partition_size_ != 0);
        CreateSLTVPEPartition();
    }

    void CreateSLTVPEPartition() {
        PliCluster const& base_partition_element = base_partition_[base_partition_value_id_];
        for (RecordIdentifier const record_id : utility::NotEmpty(base_partition_element)) {
            PartitionValueIdMap const& lt_pvid_map = lt_pvid_maps_[record_id];
            basepe_partition_[&lt_pvid_map].push_back(&lt_pvid_map);
        }
        basepe_partition_iter_ = basepe_partition_.begin();
    }

    bool NextSLTVPEPartition() {
        if (++base_partition_value_id_ == base_partition_size_) return false;
        // This method must not be called after false is returned.
        DESBORDANTE_ASSUME(base_partition_value_id_ < base_partition_size_);
        basepe_partition_.clear();
        CreateSLTVPEPartition();
        return true;
    }

    void AddConstraint(AtomicConstraintsInfo::const_iterator constraint_iter,
                       PartitionValueIdMap const& representative_lt_pvid_map) {
        AtomicConstraintInfo const& constraint = *constraint_iter;
        Index const record_match_index = constraint.record_match_index;
        PartitionValueId const lt_pvalue_id = representative_lt_pvid_map[record_match_index];
        PartitionValueId const rt_pvalue_id = record_match_index;
        other_paac_scratch_.emplace_back(&(*validator_->record_match_indexes_)[record_match_index]
                                                  .value_matrix[lt_pvalue_id],
                                         constraint.vm_rcv_id, rt_pvalue_id);
    }

    PartValueSet GetPartValueSet(AtomicConstraintInfo const& constraint,
                                 PartitionValueIdMap const& representative_lt_pvid_map) {
        Index const record_match_index = constraint.record_match_index;
        Index const lt_partition_id = constraint.record_match_index;
        RecordClassifierValueId usindex_rcv_id = constraint.usindex_rcv_id;
        return validator_->GetMatchedValuesSet(
                record_match_index, representative_lt_pvid_map[lt_partition_id], usindex_rcv_id);
    }

    void UpdateSmallestSetOrAddConstraint(AtomicConstraintsInfo::const_iterator constraint_iter,
                                          PartValueSet& current_set,
                                          PartValueSet& current_smallest_set,
                                          PartitionValueIdMap const& representative_lt_pvid_map,
                                          AtomicConstraintsInfo::const_iterator& smallest_iter) {
        if (current_set.number_of_records < current_smallest_set.number_of_records) {
            AddConstraint(smallest_iter, representative_lt_pvid_map);
            current_smallest_set = std::move(current_set);
            smallest_iter = constraint_iter;
        } else {
            AddConstraint(constraint_iter, representative_lt_pvid_map);
        }
    }

    // 1) No sorting (current).
    // 2) Save record numbers, sort as before with arrangement.
    // 3) Same as 2, but use 2x size deque-like scratch, starting inserting in the middle, and on
    // every new smallest shift starting pointer left and insert it there, sort as before with
    // arrangement.
    // 4) Add record numbers to other_paac_scratch_, sort directly.
    // 5) Same as 4, with 3's modification.
    // TODO: test performance
    std::pair<std::span<PartitionValueId const>, Index> FillConstraints(
            PartitionValueIdMap const& representative_lt_pvid_map) {
        other_paac_scratch_.clear();

        AtomicConstraintsInfo::const_iterator constraint_iter = lhs_atomic_constraints_.begin();
        AtomicConstraintsInfo::const_iterator smallest_iter = lhs_atomic_constraints_.begin();
        AtomicConstraintsInfo::const_iterator const second_to_last_iter =
                lhs_atomic_constraints_.end() - 1;
        PartValueSet current_smallest_set =
                GetPartValueSet(*constraint_iter, representative_lt_pvid_map);
        if (current_smallest_set.IsEmpty()) return {};

        while (++constraint_iter != second_to_last_iter) {
            PartValueSet current_set =
                    GetPartValueSet(*constraint_iter, representative_lt_pvid_map);
            if (current_set.IsEmpty()) return {};
            UpdateSmallestSetOrAddConstraint(constraint_iter, current_set, current_smallest_set,
                                             representative_lt_pvid_map, smallest_iter);
        }

        PartValueSet last_set = GetPartValueSet(*constraint_iter, representative_lt_pvid_map);
        if (last_set.IsEmpty()) return {};
        UpdateSmallestSetOrAddConstraint(constraint_iter, last_set, current_smallest_set,
                                         representative_lt_pvid_map, smallest_iter);

        return {current_smallest_set.values, smallest_iter->record_match_index};
    }

    bool AddUpperSet(AtomicConstraintInfo const& constraint_info,
                     PartitionValueIdMap const& representative_lt_pvid_map) {
        // atomic constraint, need record match index
        RecordClassifierValueId const usindex_rcv_id = constraint_info.usindex_rcv_id;
        Index const lt_partition_id = constraint_info.record_match_index;
        Index const record_match_index = constraint_info.record_match_index;
        PartValueSet matched_values_set = validator_->GetMatchedValuesSet(
                record_match_index, representative_lt_pvid_map[lt_partition_id], usindex_rcv_id);
        if (matched_values_set.IsEmpty()) return true;
        matched_rt_value_sets_scratch_.push_back(matched_values_set);
        return false;
    }

    // TODO: construct constraints on the fly here, keep track of the one with the lowest number of
    // records, go up to second-to-last, then check if the last has the lowest number, keep
    // other_paac_scratch_ capacity the same.
    // Get rid of matched_rt_value_sets_scratch_? Then can't sort it based on the number of records.
    bool FindMatchedRightTableValueSets(PartitionValueIdMap const& representative_lt_pvid_map) {
        matched_rt_value_sets_scratch_.clear();

        for (AtomicConstraintInfo const& constraint_info : lhs_atomic_constraints_) {
            if (AddUpperSet(constraint_info, representative_lt_pvid_map)) return true;
        }
        return false;
    }

    void CreatePAACArrangement() {
        std::span arr_span{paac_arrangement_scratch_.get(), lhs_atomic_constraints_size_};
        std::iota(arr_span.begin(), arr_span.end(), 0);
        std::ranges::sort(arr_span, [this](Index i, Index j) {
            return matched_rt_value_sets_scratch_[i].number_of_records <
                   matched_rt_value_sets_scratch_[j].number_of_records;
        });
    }

    void AddConstraint(AtomicConstraintInfo const& constraint,
                       PartitionValueIdMap const& representative_lt_pvid_map) {
        Index const lt_partition_id = constraint.record_match_index;
        Index const rt_partition_id = constraint.record_match_index;
        Index const record_match_index = constraint.record_match_index;
        RecordClassifierValueId const vm_rcv_id = constraint.vm_rcv_id;
        PartitionValueId const lt_pvalue_id = representative_lt_pvid_map[lt_partition_id];
        other_paac_scratch_.emplace_back(&(*validator_->record_match_indexes_)[record_match_index]
                                                  .value_matrix[lt_pvalue_id],
                                         vm_rcv_id, rt_partition_id);
    }

    void CreateConstraints(PartitionValueIdMap const& representative_lt_pvid_map) {
        CreatePAACArrangement();

        other_paac_scratch_.clear();
        std::span other_arrangement{&paac_arrangement_scratch_[1],
                                    lhs_atomic_constraints_size_ - 1};
        for (Index constraint_index : utility::NotEmpty(other_arrangement)) {
            AtomicConstraintInfo const& constraint = lhs_atomic_constraints_[constraint_index];
            AddConstraint(constraint, representative_lt_pvid_map);
        }
    }

    PositionListIndex const& GetIteratedRightTablePli() {
        return validator_->GetRightTablePartitionIndex().GetPli(
                lhs_atomic_constraints_[paac_arrangement_scratch_[0]].record_match_index);
    }

    auto CreateIterator() {
        RecordIdentifier first_rt_record;
        RightTableIterator iter{SatisfiesConstraintsFilter{rt_pvid_maps_, other_paac_scratch_},
                                GetIteratedRightTablePli(),
                                matched_rt_value_sets_scratch_[paac_arrangement_scratch_[0]].values,
                                &first_rt_record};
        return std::pair{std::move(iter), first_rt_record};
    }

    class SatisfiesConstraintsFilter {
        TablePartitionValueIdMaps const& rt_pvid_maps_;
        std::span<PartiallyAppliedAtomicConstraint const> constraints_;

    public:
        bool operator()(RecordIdentifier record_id) const {
            PartitionValueIdMap const& pvid_map = rt_pvid_maps_[record_id];
            DESBORDANTE_ASSUME(!constraints_.empty());
            for (PartiallyAppliedAtomicConstraint const& constraint : constraints_) {
                if (!constraint.IsSatisified(pvid_map)) return false;
            }
            return true;
        }

        SatisfiesConstraintsFilter(TablePartitionValueIdMaps const& rt_pvid_maps,
                                   std::span<PartiallyAppliedAtomicConstraint const> constraints)
            : rt_pvid_maps_(rt_pvid_maps), constraints_(constraints) {}
    };

    using Iterator = RightTableIterator<SatisfiesConstraintsFilter>;

public:
    using PartitionElement = LHSMRPartitionElementInfo<Iterator>;

    MultiCardPartitionElementProvider(BatchValidator const* validator, MdeLhs const& lhs)
        : MultiCardPartitionElementProvider(Initializer{validator, lhs}) {}

    std::optional<PartitionElement> TryGetNextLHSMRPartitionElement() {
        do {
            while (basepe_partition_iter_ != basepe_partition_.end()) {
                auto const& [representative_lt_pvid_map, basepe_partition_element_pvid_maps] =
                        *basepe_partition_iter_++;

                // auto [iterated_values, iterated_rm_index] =
                //      FillConstraints(*representative_lt_pvid_map);

                // if (iterated_values.empty()) continue;

                bool const empty_set_found =
                        FindMatchedRightTableValueSets(*representative_lt_pvid_map);
                if (empty_set_found) continue;

                CreateConstraints(*representative_lt_pvid_map);

                auto [iter, first_rt_record] = CreateIterator();
                if (first_rt_record == kNoMoreRecords) continue;

                return {{basepe_partition_element_pvid_maps, first_rt_record, std::move(iter)}};
            }
        } while (NextSLTVPEPartition());
        return std::nullopt;
    }
};

void BatchValidator::Validate(ValidationSelection& selection, Result& result,
                              SameLhsValidators& same_lhs_validators) const {
    switch (MdeLhs const& lhs = selection.updater->GetLhs(); lhs.Cardinality()) {
        [[unlikely]] case 0:
            // Already validated.
            break;
        case 1: {
            LHSMRPartitionInspector<OneCardPartitionElementProvider> inspector{
                    this, result, same_lhs_validators, lhs};
            inspector.InspectLHSMRPartition();
        } break;
        default: {
            LHSMRPartitionInspector<MultiCardPartitionElementProvider> inspector{
                    this, result, same_lhs_validators, lhs};
            inspector.InspectLHSMRPartition();
        } break;
    }
}

auto BatchValidator::GetRemovedAndInterestingness(ValidationSelection const& info,
                                                  std::vector<Index> const& indices)
        -> RemovedAndInterestingnessRCVIds {
    MdeLhs const& lhs = info.updater->GetLhs();

    std::vector<RecordClassifierValueId> interestingness_rcv_ids;
    auto set_interestingness_rcv_ids = [&](std::vector<RecordClassifierValueId>& removed_rcv_ids) {
        assert(std::ranges::all_of(removed_rcv_ids,
                                   [](RecordClassifierValueId rcv_id) { return rcv_id > 0; }));
        std::ranges::for_each(removed_rcv_ids, [](RecordClassifierValueId& rcv_id) { --rcv_id; });
        interestingness_rcv_ids = lattice_->GetInterestingnessRCVIds(lhs, indices, removed_rcv_ids);
        std::ranges::for_each(removed_rcv_ids, [](RecordClassifierValueId& rcv_id) { ++rcv_id; });
    };
    std::vector<RecordClassifierValueId> removed_rcv_ids =
            info.updater->GetRhs().DisableAndDo(indices, set_interestingness_rcv_ids);
    return {std::move(removed_rcv_ids), std::move(interestingness_rcv_ids)};
}

void BatchValidator::FillValidators(
        SameLhsValidators& same_lhs_validators, LhsGroupedRecommendations& recommendations,
        std::vector<Index> const& pending_rhs_indices,
        RemovedAndInterestingnessRCVIds const& removed_and_interestingness) const {
    {
        std::size_t const rhss_number = pending_rhs_indices.size();
        same_lhs_validators.reserve(rhss_number);
    }

    for (auto const& [removed_rcv_ids, interestingness_rcv_ids] = removed_and_interestingness;
         auto [record_match_index, old_rcv_id, interestingness_rcv_id] :
         utility::Zip(pending_rhs_indices, removed_rcv_ids, interestingness_rcv_ids)) {
        MdeElement rhs{record_match_index, old_rcv_id};
        same_lhs_validators.emplace_back(
                rhs, recommendations, GetLeftTablePartitioningValuesCount(record_match_index),
                interestingness_rcv_id, (*record_match_indexes_)[record_match_index].value_matrix);
    }
}

void BatchValidator::CreateValidators(ValidationSelection const& selection,
                                      SameLhsValidators& same_lhs_validators,
                                      LhsGroupedRecommendations& recommendations) {
    boost::dynamic_bitset<> const& pending_rhs_indices_bitset = selection.rhs_indices_to_validate;
    if (pending_rhs_indices_bitset.none()) return;

    // NOTE: converting to indices because the index list is iterated through many times in
    // MdLattice::GetInterestingnessRCVIds, and it is faster to allocate and iterate through a
    // vector than a bitset.
    std::vector<Index> const pending_rhs_indices =
            util::BitsetToIndices<Index>(pending_rhs_indices_bitset);

    RemovedAndInterestingnessRCVIds removed_and_interestingness =
            GetRemovedAndInterestingness(selection, pending_rhs_indices);

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
        Index const lhs_index, Result& result, boost::dynamic_bitset<>& rhs_indices_to_validate,
        lattice::Rhs const& lattice_rhs) {
    // If LHS has cardinality 1 and the record classifier uses a natural decision boundary, then it
    // follows that the RHS record classifier decision boundary must be equal to it, giving us a
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
void BatchValidator::CreateResult(ValidationSelection& selection) {
    Result& result = results_.emplace_back();

    // Store locally if validation is performed, store in current_validator_batch_ if postponed.
    auto&& rhs_validators = GetRhsValidatorsContainer<PerformValidation>();

    boost::dynamic_bitset<>& rhs_indices_to_validate = selection.rhs_indices_to_validate;
    lattice::Rhs& lattice_rhs = selection.updater->GetRhs();

    switch (MdeLhs const& lhs = selection.updater->GetLhs(); lhs.Cardinality()) {
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
            CreateValidators(selection, rhs_validators, result.lhs_grouped_recommendations);
            if constexpr (PerformValidation) Validate(selection, result, rhs_validators);
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
        CreateResults<true>(selections);
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

BatchValidator::BatchValidator(
        util::WorkerThreadPool* pool,
        record_match_indexes::DataPartitionIndex const* const data_partition_index,
        std::vector<record_match_indexes::Indexes> const& record_match_indexes,
        std::size_t min_support, lattice::MdeLattice* lattice,
        std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps,
        ValidatedAdder validated_adder)
    : pool_(pool),
      data_partition_index_(data_partition_index),
      record_match_indexes_(&record_match_indexes),
      min_support_(min_support),
      lattice_(lattice),
      rcv_id_lr_maps_(&rcv_id_lr_maps),
      validated_adder_(std::move(validated_adder)) {}
}  // namespace algos::hymde::cover_calculation

