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
using lattice::PathToNode;
using model::Index;
using record_match_indexes::MaterializedPValuesUpperSet;
using record_match_indexes::PartitionIndex;
using PartitionValueIdMap = PartitionIndex::PartitionValueIdMap;
using PliCluster = PartitionIndex::PliCluster;
using PositionListIndex = PartitionIndex::PositionListIndex;
using record_match_indexes::RTPValueIDs;
using record_match_indexes::UpperSetIndex;

static constexpr RecordIdentifier kNoMoreRecords = -1;

// There are several partitions that are going to be mentioned here.
// Firstly, a partition of the left table based on equality of values of partitioning functions
// (`LTPVsPartition` in code, from "left table partition values partition"). Note that we create a
// partition for each partitioning function in the preprocessing stage. Thus, for any partition
// where the key has multiple partitioning functions we can obtain the sets of the partition by
// partitioning each element of one of the function's partition (also referred to as "base
// partition") "properly" only on the other functions' results (`BasePEPartition`, "base partition
// element partition"). Any element of BasePEPartition is an element of the corresponding
// LTPVsPartition.

// Finally, partition of the set of record pairs that are matched by an LHS (`LHSMRPartition`, "LHS
// matched records partition"). Its sets can be obtained by inspecting each set of the
// LTPVsPartition with the partition key being a set of the left table partitioning functions of the
// LHS's record classifiers (if \psi is the set of record classifiers of the LHS, then the set is
// {T | \exists V,F,ord,value ((T, V, F, ord), value) \in \psi}). Along with the partition values,
// each left table's record in this set also shares the set of right table's records that are
// matched by the LHS to it with the others, if any. The Cartesian products of the sets in the
// described set pairs are the elements of LHSMRPartition.

template <typename Filter>
class RightTableIterator {
    PliCluster::const_iterator cur_rt_record_iter_;
    PliCluster::const_iterator end_rt_record_iter_;
    [[no_unique_address]] Filter filter_;
    RTPValueIDs::iterator rt_pvalue_id_iter_;
    RTPValueIDs::iterator const end_rt_pvalue_id_iter_;
    PositionListIndex const& iterated_rt_pli_;

    void StartCurrentRTClusterIteration() {
        PliCluster const& current_rt_cluster =
                utility::NotEmpty(iterated_rt_pli_[*rt_pvalue_id_iter_]);
        cur_rt_record_iter_ = current_rt_cluster.begin();
        end_rt_record_iter_ = current_rt_cluster.end();
    }

    template <bool IterGuaranteedValid = false>
    RecordIdentifier NextValidRecordInternal() {
        if constexpr (IterGuaranteedValid) goto cur_is_valid;
        while (true) {
            if (cur_rt_record_iter_ == end_rt_record_iter_) {
                if (++rt_pvalue_id_iter_ == end_rt_pvalue_id_iter_) return kNoMoreRecords;
                StartCurrentRTClusterIteration();
            }

        cur_is_valid:
            RecordIdentifier potential_record_id = *cur_rt_record_iter_++;
            if (filter_(potential_record_id)) {
                DESBORDANTE_ASSUME(potential_record_id != kNoMoreRecords);
                return potential_record_id;
            }
        }
    }

public:
    RightTableIterator(Filter filter, PositionListIndex const& iterated_rt_pli_,
                       RTPValueIDs iterated_rt_pvalue_ids, RecordIdentifier* first_rt_record_id)
        : filter_(std::move(filter)),
          rt_pvalue_id_iter_(iterated_rt_pvalue_ids.begin()),
          end_rt_pvalue_id_iter_(iterated_rt_pvalue_ids.end()),
          iterated_rt_pli_(iterated_rt_pli_) {
        StartCurrentRTClusterIteration();
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
    bool InspectRTRecord(PartitionValueIdMap const& rt_record_pvid_map) {
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
                            SameLhsValidators& rhs_validators, PathToNode const& lhs)
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
                [this](PartitionValueIdMap const& rt_pvid_map) {
                    return InspectRTRecord(rt_pvid_map);
                },
                [this]() { MakeOutOfClustersResult(); });
    }
};

class BatchValidator::OneCardPartitionElementProvider {
    static constexpr auto kNoFilter = [](RecordIdentifier) { return true; };
    using RTIterator = RightTableIterator<decltype(kNoFilter)>;

    PartitionValueId current_lt_pvalue_id_ = PartitionValueId(-1);
    Index const lhs_record_match_index_;
    RecordClassifierValueId const lhs_rcv_id_;
    PositionListIndex const& lt_pli_;
    std::size_t const lt_pvalues_ = lt_pli_.size();
    PositionListIndex const& rt_pli_;
    TablePartitionValueIdMaps const& lt_pvid_maps_;
    UpperSetIndex const& upper_set_index_;
    // TODO: remember largest cluster size for each PLI, move to std::unique_ptr.
    LTPVsPEPVIdMaps ltpvsp_element_pvid_maps_scratch_;

    void FillLTPVsPEScratch() {
        ltpvsp_element_pvid_maps_scratch_.clear();
        PliCluster const& lt_records = lt_pli_[current_lt_pvalue_id_];
        utility::ReserveMore(ltpvsp_element_pvid_maps_scratch_, lt_records.size());
        for (RecordIdentifier lt_record_id : utility::NotEmpty(lt_records)) {
            ltpvsp_element_pvid_maps_scratch_.push_back(&lt_pvid_maps_[lt_record_id]);
        }
    }

public:
    using PartitionElement = LHSMRPartitionElementInfo<RTIterator>;

    OneCardPartitionElementProvider(BatchValidator const* validator, PathToNode const& lhs)
        : lhs_record_match_index_(lhs.begin()->offset),
          lhs_rcv_id_(lhs.begin()->rcv_id),
          lt_pli_(validator->GetLeftTablePartitionIndex().GetPli(lhs_record_match_index_)),
          rt_pli_(validator->GetRightTablePartitionIndex().GetPli(lhs_record_match_index_)),
          lt_pvid_maps_(validator->GetLeftTablePartitionIndex().GetPartitionValueIdMaps()),
          upper_set_index_(
                  (*validator->record_match_indexes_)[lhs_record_match_index_].upper_set_index) {
        assert(lhs.PathLength() == 1);
    }

    std::optional<PartitionElement> TryGetNextLHSMRPartitionElement() {
        while (++current_lt_pvalue_id_ != lt_pvalues_) {
            MaterializedPValuesUpperSet upper_set =
                    upper_set_index_[current_lt_pvalue_id_].GetUpperSet(lhs_rcv_id_);
            if (upper_set.IsEmpty()) continue;

            FillLTPVsPEScratch();

            RecordIdentifier first_rt_record_id;
            RTIterator iter{kNoFilter, rt_pli_, upper_set.value_set_elements, &first_rt_record_id};
            return {{ltpvsp_element_pvid_maps_scratch_, first_rt_record_id, std::move(iter)}};
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

        PositionListIndex const* base_partition_;

        PathToNode::iterator lhs_iter_;
        PathToNode::iterator const lhs_end_;

        Index cur_rm_index_ = lhs_iter_->offset;

        // TODO: permute record matches for validation so that this partition is the one with the
        // largest amount of clusters or with the smallest maximum cluster size.
        std::vector<Index> basepe_partition_key_indices_;

        AtomicConstraintsInfo lhs_atomic_constraints_;

        void AddConstraint(RecordClassifierValueId const lhs_rcv_id) {
            lhs_atomic_constraints_.emplace_back(
                    cur_rm_index_, lhs_rcv_id,
                    rcv_id_lr_maps_[cur_rm_index_].lhs_to_rhs_map[lhs_rcv_id]);
        }

        void AddFirstMatchingCriterion() {
            Index base_partition_index =
                    cur_rm_index_;  // rm_indices[cur_rec_match_index_].lt_partition_id
            base_partition_ =
                    &validator_->GetLeftTablePartitionIndex().GetPli(base_partition_index);

            // LHS has cardinality greater than 1, so is not empty.
            DESBORDANTE_ASSUME(lhs_iter_ != lhs_end_);
            AddConstraint(lhs_iter_->rcv_id);
            ++cur_rm_index_;
            ++lhs_iter_;
        }

        void AddOtherMatchingCriteria() {
            auto lhs_tail = std::span{lhs_iter_, lhs_end_};
            // LHS has cardinality greater than 1, so it has at least one more element.
            for (auto const& [next_node_offset, lhs_rcv_id] : utility::NotEmpty(lhs_tail)) {
                cur_rm_index_ += next_node_offset;
                basepe_partition_key_indices_.push_back(
                        cur_rm_index_);  // rm_indices[cur_rm_index_].lt_partition_id
                AddConstraint(lhs_rcv_id);
                ++cur_rm_index_;
            }
        }

    public:
        Initializer(BatchValidator const* validator, PathToNode const& lhs)
            : validator_(validator), lhs_iter_(lhs.begin()), lhs_end_(lhs.end()) {
            std::size_t const cardinality = lhs.PathLength();
            // This class is adapted for this case and should only be created if this holds.
            DESBORDANTE_ASSUME(cardinality > 1);
            basepe_partition_key_indices_.reserve(cardinality - 1);
            lhs_atomic_constraints_.reserve(cardinality);

            AddFirstMatchingCriterion();
            AddOtherMatchingCriteria();
            // Sort (and apply std::unique) if not for better locality, currently should be sorted,
            // as these are just record match indices.
            assert(std::ranges::is_sorted(basepe_partition_key_indices_));
        }

        BatchValidator const* GetValidator() const noexcept {
            return validator_;
        }

        std::vector<Index>&& GetBasePEPartitionKeyIndices() noexcept {
            return std::move(basepe_partition_key_indices_);
        }

        AtomicConstraintsInfo&& TakeConstraints() noexcept {
            return std::move(lhs_atomic_constraints_);
        }

        PositionListIndex const& GetPositionListIndex() const noexcept {
            return *base_partition_;
        }
    };

    class IndexHasher {
        std::span<Index> const indices_;

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
        std::span<Index> const indices_;

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
        // These fields are always copied, can this be avoided? Should it be?
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

    class SatisfiesConstraintsFilter {
        TablePartitionValueIdMaps const& rt_pvid_maps_;
        std::span<PartiallyAppliedAtomicConstraint const> constraints_;

    public:
        bool operator()(RecordIdentifier record_id) const {
            PartitionValueIdMap const& rt_pvid_map = rt_pvid_maps_[record_id];
            for (PartiallyAppliedAtomicConstraint const& constraint :
                 utility::NotEmpty(constraints_)) {
                if (!constraint.IsSatisified(rt_pvid_map)) return false;
            }
            return true;
        }

        SatisfiesConstraintsFilter(TablePartitionValueIdMaps const& rt_pvid_maps,
                                   std::span<PartiallyAppliedAtomicConstraint const> constraints)
            : rt_pvid_maps_(rt_pvid_maps), constraints_(constraints) {}
    };

    class LTPVsPartitionIterator {
        PositionListIndex::const_iterator base_partition_iterator_;
        PositionListIndex::const_iterator const base_partition_end_;

        std::vector<Index> basepe_partition_key_indices_;

        GroupMap basepe_partition_{0, IndexHasher(basepe_partition_key_indices_),
                                   IndexComparer(basepe_partition_key_indices_)};
        GroupMap::iterator basepe_partition_iter_ = basepe_partition_.begin();

        TablePartitionValueIdMaps const& lt_pvid_maps_;

        void CreateBasePEPartition() {
            PliCluster const& base_partition_element = *base_partition_iterator_;
            for (RecordIdentifier const record_id : utility::NotEmpty(base_partition_element)) {
                PartitionValueIdMap const& lt_pvid_map = lt_pvid_maps_[record_id];
                basepe_partition_[&lt_pvid_map].push_back(&lt_pvid_map);
            }
            basepe_partition_iter_ = basepe_partition_.begin();
        }

        bool NextBasePEPartition() {
            if (++base_partition_iterator_ == base_partition_end_) return false;

            basepe_partition_.clear();
            CreateBasePEPartition();
            return true;
        }

    public:
        LTPVsPartitionIterator(PositionListIndex const& base_partition,
                               std::vector<Index> basepe_partition_key_indices,
                               TablePartitionValueIdMaps const& lt_pvid_maps)
            : base_partition_iterator_(base_partition.begin()),
              base_partition_end_(base_partition.end()),
              basepe_partition_key_indices_(std::move(basepe_partition_key_indices)),
              lt_pvid_maps_(lt_pvid_maps) {
            // Tables are guaranteed to not be empty, so a partition can't be empty either.
            DESBORDANTE_ASSUME(base_partition_iterator_ != base_partition_end_);
            CreateBasePEPartition();
        }

        GroupMap::value_type* NextLTPVsPartitionElementPtr() {
            if (basepe_partition_iter_ == basepe_partition_.end()) {
                if (!NextBasePEPartition()) return nullptr;
                assert(basepe_partition_iter_ != basepe_partition_.end());
            }
            return &*basepe_partition_iter_++;
        }
    };

    class RightTableIteratorBuilder {
        struct SortInfo {
            std::size_t number_of_records;
            PartiallyAppliedAtomicConstraint paac;

            friend bool operator<(SortInfo const& first, SortInfo const& second) noexcept {
                return first.number_of_records < second.number_of_records;
            }
        };

        class PAACSortScratchBuilder {
            AtomicConstraintsInfo::const_iterator cur_constraint_iter_;
            AtomicConstraintsInfo::const_iterator const end_iter_;

            AtomicConstraintsInfo::const_iterator least_records_upper_set_constraint_iter_ =
                    cur_constraint_iter_;

            std::vector<record_match_indexes::Indexes> const* const record_match_indexes_;

            PartitionValueIdMap const& representative_lt_pvid_map_;

            SortInfo* current_paac_;

            MaterializedPValuesUpperSet least_records_upper_set_ = GetCurrentUpperSet();

            void AddPAAC(AtomicConstraintsInfo::const_iterator constraint_iter,
                         std::size_t record_num) {
                AtomicConstraintInfo const& constraint = *constraint_iter;
                Index const lt_partition_id = constraint.record_match_index;
                Index const rt_partition_id = constraint.record_match_index;
                Index const record_match_index = constraint.record_match_index;
                PartitionValueId const lt_pvalue_id = representative_lt_pvid_map_[lt_partition_id];
                record_match_indexes::ValueMatrix const& constraint_rm_value_matrix =
                        (*record_match_indexes_)[record_match_index].value_matrix;
                RecordClassifierValueId const vm_rcv_id = constraint.vm_rcv_id;
                *current_paac_++ = {
                        record_num,
                        {&constraint_rm_value_matrix[lt_pvalue_id], vm_rcv_id, rt_partition_id}};
            }

        public:
            PAACSortScratchBuilder(
                    AtomicConstraintsInfo const& lhs_atomic_constraints,
                    std::vector<record_match_indexes::Indexes> const* record_match_indexes,
                    PartitionValueIdMap const& representative_lt_pvid_map,
                    std::unique_ptr<SortInfo[]> const& paac_sort_scratch)
                : cur_constraint_iter_(lhs_atomic_constraints.begin()),
                  end_iter_(lhs_atomic_constraints.end()),
                  record_match_indexes_(record_match_indexes),
                  representative_lt_pvid_map_(representative_lt_pvid_map),
                  current_paac_(paac_sort_scratch.get()) {}

            bool MoveNext() {
                return ++cur_constraint_iter_ != end_iter_;
            }

            MaterializedPValuesUpperSet const& GetLeastRecordsPVSet() const noexcept {
                return least_records_upper_set_;
            }

            MaterializedPValuesUpperSet GetCurrentUpperSet() const {
                AtomicConstraintInfo const& constraint = *cur_constraint_iter_;
                Index const record_match_index = constraint.record_match_index;
                Index const lt_partition_id = constraint.record_match_index;
                PartitionValueId const lt_pvalue_id = representative_lt_pvid_map_[lt_partition_id];
                RecordClassifierValueId const usindex_rcv_id = constraint.usindex_rcv_id;
                return (*record_match_indexes_)[record_match_index]
                        .upper_set_index[lt_pvalue_id]
                        .GetUpperSet(usindex_rcv_id);
            }

            void Process(MaterializedPValuesUpperSet&& upper_set) {
                if (upper_set.record_set_cardinality <
                    least_records_upper_set_.record_set_cardinality) {
                    AddPAAC(least_records_upper_set_constraint_iter_,
                            least_records_upper_set_.record_set_cardinality);
                    least_records_upper_set_ = std::move(upper_set);
                    least_records_upper_set_constraint_iter_ = cur_constraint_iter_;
                } else {
                    AddPAAC(cur_constraint_iter_, upper_set.record_set_cardinality);
                }
            }

            Index GetLeastRecordsUpperSetRecordMatchIndex() const noexcept {
                return least_records_upper_set_constraint_iter_->record_match_index;
            }
        };

        std::vector<record_match_indexes::Indexes> const* const record_match_indexes_;

        PartitionIndex const& rt_partition_index_;

        AtomicConstraintsInfo const lhs_atomic_constraints_;
        std::size_t const paac_number_ = lhs_atomic_constraints_.size() - 1;

        std::unique_ptr<SortInfo[]> paac_sort_scratch_ =
                utility::MakeUniqueForOverwrite<SortInfo[]>(paac_number_);

        std::unique_ptr<PartiallyAppliedAtomicConstraint[]> const sorted_paacs_scratch_ =
                utility::MakeUniqueForOverwrite<PartiallyAppliedAtomicConstraint[]>(paac_number_);

        void MakeSortedConstraints() {
            auto paac_info_array = std::span{paac_sort_scratch_.get(), paac_number_};
            std::ranges::sort(paac_info_array, std::less{});
            PartiallyAppliedAtomicConstraint* cur_slot = sorted_paacs_scratch_.get();
            for (auto const& [number_of_records, paac] : paac_info_array) {
                *cur_slot++ = paac;
            }
        }

    public:
        RightTableIteratorBuilder(
                std::vector<record_match_indexes::Indexes> const* record_match_indexes,
                PartitionIndex const& rt_partition_index,
                AtomicConstraintsInfo lhs_atomic_constraints)
            : record_match_indexes_(record_match_indexes),
              rt_partition_index_(rt_partition_index),
              lhs_atomic_constraints_(std::move(lhs_atomic_constraints)) {
            assert(lhs_atomic_constraints_.size() >= 2);
        }

        // TODO: test performance
        // 1) No sorting.
        // 2) Decorate PAAC payload, sort.
        // 3) Same as 2, but use 2x size deque-like scratch, starting inserting in the middle, and
        // on every new smallest shift starting pointer left and insert it there, sort.
        // 4) Sort PAACs and keys together (no copy).
        // 5) Same as 4, with 3's modification.
        // The sorting cost is probably negligible.
        std::pair<RTPValueIDs, Index> FindIteratedUpperSetAndBuildPAACs(
                PartitionValueIdMap const& representative_lt_pvid_map) {
            PAACSortScratchBuilder builder{lhs_atomic_constraints_, record_match_indexes_,
                                           representative_lt_pvid_map, paac_sort_scratch_};
            if (builder.GetLeastRecordsPVSet().IsEmpty()) return {};
            builder.MoveNext();

            do {
                MaterializedPValuesUpperSet current_upper_set = builder.GetCurrentUpperSet();
                if (current_upper_set.IsEmpty()) return {};
                builder.Process(std::move(current_upper_set));
            } while (builder.MoveNext());

            MakeSortedConstraints();

            MaterializedPValuesUpperSet const& least_records_pv_set =
                    builder.GetLeastRecordsPVSet();
            DESBORDANTE_ASSUME(!least_records_pv_set.value_set_elements.empty());
            return {least_records_pv_set.value_set_elements,
                    builder.GetLeastRecordsUpperSetRecordMatchIndex()};
        }

        auto BuildIterator(RTPValueIDs iterated_rt_pvalue_ids, Index rt_partition_id) {
            RecordIdentifier first_rt_record_id;
            RightTableIterator iter{
                    SatisfiesConstraintsFilter{rt_partition_index_.GetPartitionValueIdMaps(),
                                               {sorted_paacs_scratch_.get(), paac_number_}},
                    rt_partition_index_.GetPli(rt_partition_id), iterated_rt_pvalue_ids,
                    &first_rt_record_id};
            return std::pair{std::move(iter), first_rt_record_id};
        }
    };

    LTPVsPartitionIterator ltpvs_partition_iter_;
    RightTableIteratorBuilder rt_iterator_builder_;

    MultiCardPartitionElementProvider(Initializer init_info)
        : ltpvs_partition_iter_(
                  init_info.GetValidator()
                          ->data_partition_index_->GetRight()
                          .GetPartitionValueIdMaps(),
                  init_info.GetBasePEPartitionKeyIndices(),
                  init_info.GetValidator()->GetLeftTablePartitionIndex().GetPartitionValueIdMaps()),
          rt_iterator_builder_(init_info.GetValidator()->record_match_indexes_,
                               init_info.GetValidator()->GetRightTablePartitionIndex(),
                               std::move(init_info.TakeConstraints())) {}

public:
    using PartitionElement =
            LHSMRPartitionElementInfo<RightTableIterator<SatisfiesConstraintsFilter>>;

    MultiCardPartitionElementProvider(BatchValidator const* validator, PathToNode const& lhs)
        : MultiCardPartitionElementProvider(Initializer{validator, lhs}) {}

    std::optional<PartitionElement> TryGetNextLHSMRPartitionElement() {
        while (GroupMap::value_type* basepe_partition_element_info =
                       ltpvs_partition_iter_.NextLTPVsPartitionElementPtr()) {
            auto const& [representative_lt_pvid_map, basepe_partition_element_pvid_maps] =
                    *basepe_partition_element_info;

            auto [iterated_values, iterated_rm_index] =
                    rt_iterator_builder_.FindIteratedUpperSetAndBuildPAACs(
                            *representative_lt_pvid_map);

            bool const empty_value_set_found = iterated_values.empty();
            if (empty_value_set_found) continue;

            auto [iter, first_rt_record_id] =
                    rt_iterator_builder_.BuildIterator(iterated_values, iterated_rm_index);
            if (first_rt_record_id == kNoMoreRecords) continue;

            return {{basepe_partition_element_pvid_maps, first_rt_record_id, std::move(iter)}};
        }
        return std::nullopt;
    }
};

void BatchValidator::Validate(ValidationSelection& selection, Result& result,
                              SameLhsValidators& same_lhs_validators) const {
    switch (PathToNode const& lhs = selection.updater->GetLhs(); lhs.PathLength()) {
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
    PathToNode const& lhs = info.updater->GetLhs();

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

    switch (PathToNode const& lhs = selection.updater->GetLhs(); lhs.PathLength()) {
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

std::vector<RecordClassifierIdentifiers> ValidatedAdder::MakeLhsRCIdentifierArray(
        PathToNode const& lattice_lhs) const {
    std::vector<RecordClassifierIdentifiers> lhs_rm_specification =
            util::GetPreallocatedVector<RecordClassifierIdentifiers>(lattice_lhs.PathLength());
    Index total_offset = 0;
    for (auto const& [offset, lhs_rcv_id] : utility::NotEmpty(lattice_lhs)) {
        total_offset += offset;
        RecordClassifierValueId rcv_id = rcv_id_lr_maps_[total_offset].lhs_to_rhs_map[lhs_rcv_id];
        Index const translated_rm_index = reordered_to_original_rm_map_[total_offset];
        lhs_rm_specification.emplace_back(translated_rm_index, rcv_id);
        ++total_offset;
    }
    return lhs_rm_specification;
}

void ValidatedAdder::ResetBitsForRemoved(boost::dynamic_bitset<>& selected_rhs_indices,
                                         ValidationRhsUpdates const& invalidated) const {
    for (auto const& [index, new_rcv_id] : invalidated.GetUpdateView()) {
        if (new_rcv_id == kLowestRCValueId) selected_rhs_indices.reset(index);
    }
}

std::vector<RecordClassifierIdentifiers> ValidatedAdder::MakeRhsRCIdentifierArray(
        ValidationRhsUpdates const& invalidated, boost::dynamic_bitset<>& rhs_indices_to_validate,
        std::size_t dependencies_number_for_lhs, lattice::Rhs const& rhs) const {
    assert(dependencies_number_for_lhs == rhs_indices_to_validate.count());
    // Reserve only as much as needed, as there can be many dependencies.
    std::vector<RecordClassifierIdentifiers> rhs_rm_specification =
            util::GetPreallocatedVector<RecordClassifierIdentifiers>(dependencies_number_for_lhs);
    // Add lowered.
    for (auto const& [reordered_index, new_rcv_id] : invalidated.GetUpdateView()) {
        if (new_rcv_id == kLowestRCValueId) continue;
        DESBORDANTE_ASSUME(rhs_indices_to_validate.test(reordered_index));
        rhs_indices_to_validate.reset(reordered_index);
        Index const translated_rm_index = reordered_to_original_rm_map_[reordered_index];
        rhs_rm_specification.emplace_back(translated_rm_index, new_rcv_id);
    }

    // Add confirmed.
    util::ForEachIndex(rhs_indices_to_validate, [&](Index reordered_index) {
        Index const original_rm_index = reordered_to_original_rm_map_[reordered_index];
        RecordClassifierValueId const rcv_id = rhs[reordered_index];
        assert(rcv_id != kLowestRCValueId);
        rhs_rm_specification.emplace_back(original_rm_index, rcv_id);
    });
    return rhs_rm_specification;
}

void ValidatedAdder::AddMDEs(ValidationSelection& selection,
                             ValidationRhsUpdates const& invalidated, std::size_t support) {
    PathToNode const& lhs = selection.updater->GetLhs();
    // Already added.
    if (lhs.PathLength() == 0) [[unlikely]]
        return;

    std::vector<RecordClassifierIdentifiers> lhs_rc_identifiers = MakeLhsRCIdentifierArray(lhs);

    boost::dynamic_bitset<>& selected_rhs_indices = selection.rhs_indices_to_validate;
    // The only bit corresponding to a removed dependency that was reset was the one in
    // RemoveTrivialForCardinality1Lhs. Reset the other bits for other removed dependencies.
    ResetBitsForRemoved(selected_rhs_indices, invalidated);

    std::size_t const dependencies_number_for_lhs = selected_rhs_indices.count();
    if (dependencies_number_for_lhs == 0) return;

    std::vector<RecordClassifierIdentifiers> rhs_rc_identifiers =
            MakeRhsRCIdentifierArray(invalidated, selected_rhs_indices, dependencies_number_for_lhs,
                                     selection.updater->GetRhs());
    assert(!rhs_rc_identifiers.empty());

    mde_specifications_.emplace_back(LhsSpecification{std::move(lhs_rc_identifiers), support},
                                     std::move(rhs_rc_identifiers));
}

auto BatchValidator::ValidateBatch(std::vector<ValidationSelection>& selections)
        -> std::vector<Result> const& {
    if (SingleThreaded()) {
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
