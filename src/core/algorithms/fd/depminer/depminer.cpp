#include "core/algorithms/fd/depminer/depminer.h"

#include <iterator>
#include <list>
#include <memory>

#include "core/algorithms/fd/bitset_result_reporter.h"
#include "core/algorithms/fd/make_plain_lhs_mask_adder.h"
#include "core/config/max_lhs/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/model/table/compute_agree_sets.h"
#include "core/util/bitset_utils.h"

namespace {
using FdLhsCandidate = model::AttributeMask;
using Level = std::unordered_set<FdLhsCandidate>;
using AgreeSet = model::AttributeMask;
using AgreeSetsSet = model::AttributeMaskSet;
using AgreeSetsVec = std::vector<AgreeSet>;
using MaximalSets = std::vector<AgreeSet>;
using NonFdLhsColumns = std::vector<model::Index>;
using algos::fd::BitsetResultReporter;

void RemoveSubsets(AgreeSet const& agree_set, MaximalSets& maximal_sets, MaximalSets::iterator it) {
    while (it != maximal_sets.end()) {
        AgreeSet const& existing_set = *it;
        if (existing_set.is_subset_of(agree_set)) {
            *it = std::move(maximal_sets.back());
            maximal_sets.pop_back();
        } else {
            ++it;
        }
    }
}

void AddToMaximalSets(MaximalSets& maximal_sets, AgreeSet const& agree_set) {
    for (auto it = maximal_sets.begin(); it != maximal_sets.end(); ++it) {
        AgreeSet const& existing_set = *it;
        if (agree_set.is_subset_of(existing_set)) {
            // If the current agree set is a (proper) subset of one of the ones in
            // maximal_sets, then it cannot be a superset of any of them.
            return;
        }
        if (existing_set.is_subset_of(agree_set)) {
            // No set in maximal_sets is a (proper) subset of any other.
            *it = std::move(maximal_sets.back());
            maximal_sets.pop_back();
            // A superset of a set in maximal_sets cannot be a subset of another set in
            // maximal_sets.
            RemoveSubsets(agree_set, maximal_sets, it);
            break;
        }
    }
    maximal_sets.push_back(agree_set);
}

MaximalSets ComputeMaximalSets(AgreeSetsVec const& agree_sets, model::Index column_index) {
    MaximalSets maximal_sets;

    for (AgreeSet const& agree_set : agree_sets) {
        // A maximal set is an attribute set X which, for some attribute A, is the largest
        // possible set not determining A.
        if (agree_set.test(column_index)) continue;

        AddToMaximalSets(maximal_sets, agree_set);
    }
    return maximal_sets;
}

std::pair<boost::dynamic_bitset<>, boost::dynamic_bitset<>> ComputeMaximalSetsUnionAndIntersection(
        MaximalSets const& maximal_sets) {
    // The original article defines the first level of LHS candidates as
    // L_1 := {{B} | ∃X ∈ cmax(dep(r), A) B ∈ X}, where cmax is the set of maximal set complements.
    // This can be implemented as a union of all maximal set complements. Equivalently, it's also
    // the complement of the intersection of all maximal sets:
    // {{B} | ∃X ∈ cmax(dep(r), A) B ∈ X} = {{B} | ∃X ∈ max(dep(r), A) B ∈ (R ∖ X)}
    // L'_1 := {B | ∃X ∈ max(dep(r), A) B ∈ (R ∖ X)} = ∪_{X ∈ max(dep(r), A)} (R ∖ X)
    // = R ∖ ∩_{X ∈ max(dep(r), A)} X = R ∖ ∩ max(dep(r), A)

    // The set of FD LHSs is LHS_i[A] := {l ∈ L_i | ∀X ∈ cmax(dep(r), A) l ∩ X != ∅}
    // Observe, that in the case of the first level this reduces to:
    // LHS_1[A] = {l ∈ L_1 | ∀X ∈ cmax(dep(r), A) l ∩ X != ∅}
    // = {{B} ∈ L_1 | ∀X ∈ cmax(dep(r), A) {B} ∩ X != ∅} =
    // = {{B} ∈ L_1 | ∀X ∈ cmax(dep(r), A) B ∈ X} = {{B} ∈ L_1 | ∀X ∈ max(dep(r), A) B ∈ (R ∖ X)}
    // LHS'_1[A] = {B ∈ L'_1 | ∀X ∈ max(dep(r), A) B ∈ (R ∖ X)}
    // = L'_1 ∩ ∩_{X ∈ max(dep(r), A)} (R ∖ X) = L'_1 ∩ (R ∖ ∪_{X ∈ max(dep(r), A)} X)
    // = (R ∖ ∩_{X ∈ max(dep(r), A)} X) ∩ (R ∖ ∪_{X ∈ max(dep(r), A)} X)
    // = R ∖ (∩_{X ∈ max(dep(r), A)} X ∪ ∪_{X ∈ max(dep(r), A)} X)
    // = R ∖ ∪_{X ∈ max(dep(r), A)} X = R ∖ ∪ max(dep(r), A)

    assert(!maximal_sets.empty());
    auto it = maximal_sets.begin();
    boost::dynamic_bitset<> set_union = *it;
    boost::dynamic_bitset<> set_intersection = *it;
    for (++it; it != maximal_sets.end(); ++it) {
        set_union |= *it;
        set_intersection &= *it;
    }
    return {std::move(set_union), std::move(set_intersection)};
    // FD LHSs are ~set_union (except the RHS column)

    // The candidates left for next level construction are L'_1 \ LHS'_1[A], so
    // (R ∖ ∩ max(dep(r), A)) ∖ (R ∖ ∪ max(dep(r), A))
    // which is ~set_intersection - ~set_union
    // ~A - ~B truth table: 0 0 -> 0, 0 1 -> 1, 1 0 -> 0, 1 1 -> 0
    // => set_union & ~set_intersection => set_union - set_intersection
}

void ReportFdLhssSingleColumn(boost::dynamic_bitset<> const& fd_lhs_columns_complement,
                              BitsetResultReporter const& report_fd_lhs) {
    FdLhsCandidate single_column_lhs(fd_lhs_columns_complement.size());
    for (model::Index fd_lhs_column = fd_lhs_columns_complement.find_first_off();
         fd_lhs_column != boost::dynamic_bitset<>::npos;
         fd_lhs_column = fd_lhs_columns_complement.find_next_off(fd_lhs_column)) {
        single_column_lhs.set(fd_lhs_column);
        report_fd_lhs(single_column_lhs);
        single_column_lhs.reset(fd_lhs_column);
    }
}

Level CreateSecondLevel(NonFdLhsColumns const& non_fd_lhs_columns, std::size_t const num_columns) {
    assert(non_fd_lhs_columns.size() >= 2);
    Level second_level;
    auto it1 = non_fd_lhs_columns.begin();
    auto it2 = std::next(it1);
    do {
        model::Index const col1 = *it1;
        it1 = it2;
        do {
            model::Index col2 = *it2;
            assert(col1 != col2);
            FdLhsCandidate lhs_candidate(num_columns);
            lhs_candidate.set(col1);
            lhs_candidate.set(col2);
            second_level.insert(std::move(lhs_candidate));
        } while (++it2 != non_fd_lhs_columns.end());
    } while ((it2 = std::next(it1)) != non_fd_lhs_columns.end());
    return second_level;
}

void FindFdLhss(Level& lhs_candidates, MaximalSets const& maximal_sets,
                BitsetResultReporter const& report_fd_lhs) {
    for (auto it = lhs_candidates.begin(); it != lhs_candidates.end();) {
        // This looks really SIMD-able.
        FdLhsCandidate const& cur_fd_lhs_candidate = *it;
        bool is_fd_lhs = true;
        for (AgreeSet const& maximal_set : maximal_sets) {
            // !cur_fd_lhs_candidate.intersects(maximal_set_complement) for complement
            if (cur_fd_lhs_candidate.is_subset_of(maximal_set)) {
                // If we take a pair of records where the RHS attribute values are not equal and see
                // that the values in the LHS columns are, this pair of records violates the FD.
                // Makes sense, IMO a lot more sense than what the article suggests with
                // complements, intersections, and all that chicanery.
                is_fd_lhs = false;
                break;
            }
        }
        if (is_fd_lhs) {
            report_fd_lhs(cur_fd_lhs_candidate);
            it = lhs_candidates.erase(it);
        } else {
            ++it;
        }
    }
}

void TryAddCandidate(FdLhsCandidate& inner_candidate_copy, FdLhsCandidate& outer_candidate_copy,
                     Level const& prev_level_candidates, Level& next_level_candidates) {
    // Level k contains sets that have all of their subsets of k - 1 length in the previous level.
    // Which means that all of these subsets can be constructed as a union of a pair of subsets from
    // the previous level and we will encounter all of these possible decompositions here.
    // This means we can pick any decomposition we like and ignore all the others.
    // Here I chose to use decompositions with equal tails because this was easy to implement (see
    // below).
    model::Index set_bit_outer = outer_candidate_copy.find_first();
    model::Index set_bit_inner = inner_candidate_copy.find_first();
    outer_candidate_copy.reset(set_bit_outer);
    inner_candidate_copy.reset(set_bit_inner);
    if (outer_candidate_copy != inner_candidate_copy) {
        outer_candidate_copy.set(set_bit_outer);
        return;
    }
    outer_candidate_copy.set(set_bit_outer);

    // Direct check that all subsets are present in prev_level_candidates.
    inner_candidate_copy.set(set_bit_inner);
    inner_candidate_copy.set(set_bit_outer);
    // We've gotten {inner,outer}_candidate from the previous level, so we already know they're
    // there, skip the first two set bits in the next level set.
    model::Index const tail_eq_bit = std::max(set_bit_outer, set_bit_inner);
    model::Index index = inner_candidate_copy.find_next(tail_eq_bit);
    assert(index != FdLhsCandidate::npos);
    do {
        inner_candidate_copy.reset(index);
        if (!prev_level_candidates.contains(inner_candidate_copy)) return;
        inner_candidate_copy.set(index);
    } while ((index = inner_candidate_copy.find_next(index)) != FdLhsCandidate::npos);
    next_level_candidates.insert(std::move(inner_candidate_copy));
}

// The article specifies Apriori-gen as the function to generate the sets, but Apriori-gen
// is specified in the form of SQL query in "Fast Algorithms for Mining Association Rules in
// Large Databases", so we can get pretty clever with the actual implementation.
// See also the apriori-gen implementation in efficient-apriori. That implementation is even better
// because it sorts the candidates by prefix for even faster generation but I don't want to figure
// out how to do it right for now.
Level ComputeNextLevelLhsCandidates(Level const& prev_level_candidates) {
    assert(prev_level_candidates.size() >= 2);
    Level next_level_candidates;

    // Having the objects defined here avoids extra allocations, operator= ends up being a simple
    // copy if a new set was not added.
    FdLhsCandidate outer_candidate_copy;
    FdLhsCandidate inner_candidate_copy;

    Level::const_iterator it1 = prev_level_candidates.begin();
    Level::const_iterator it2 = std::next(it1);
    do {
        // We really need to have our own data structures, I would have loved to use outer_candidate
        // as a buffer but have to copy.
        // I guess I can copy those to a vector, but that's extra memory consumption.
        outer_candidate_copy = *it1;
        it1 = it2;
        assert(it2 != prev_level_candidates.end());
        do {
            inner_candidate_copy = *it2++;

            // Sanity check
            assert(outer_candidate_copy != inner_candidate_copy);

            // Generation is level-wise.
            // Also, we're doing some bit pinching to avoid allocations, check that we have not
            // messed up.
            assert(outer_candidate_copy.count() == inner_candidate_copy.count());

            // We use a procedure without pointless checks when .count() == 1.
            assert(outer_candidate_copy.count() >= 2 && inner_candidate_copy.count() >= 2);

            TryAddCandidate(inner_candidate_copy, outer_candidate_copy, prev_level_candidates,
                            next_level_candidates);
        } while (it2 != prev_level_candidates.end());
    } while ((it2 = std::next(it1)) != prev_level_candidates.end());
    return next_level_candidates;
}
}  // namespace

namespace algos::fd {

using boost::dynamic_bitset, std::make_shared, std::shared_ptr, std::setw, std::vector, std::list,
        std::dynamic_pointer_cast;

Depminer::Depminer() {
    RegisterOptions();
}

void Depminer::RegisterOptions() {
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
}

void Depminer::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kMaxLhsOpt.GetName()});
}

void Depminer::ResetState() {
    fd_view_ = nullptr;
}

void Depminer::ExecuteInternal() {
    AgreeSetsSet agree_sets_set = model::ComputeAgreeSets(input_table_column_plis_);
    // Move to a vector for faster iteration
    AgreeSetsVec const agree_sets_vec{std::make_move_iterator(agree_sets_set.begin()),
                                      std::make_move_iterator(agree_sets_set.end())};

    std::size_t const column_num = input_table_column_plis_.size();
    LhsMaskFdView::Storage lhs_masks;
    lhs_masks.reserve(column_num);

    for (model::Index rhs_column_index = 0; rhs_column_index != column_num; ++rhs_column_index) {
        auto report_fd_lhs = MakePlainLhsMaskAdder(lhs_masks.emplace_back());
        // LHS size 0 is a special case.
        if (input_table_column_plis_[rhs_column_index].IsConstant()) {
            report_fd_lhs(FdLhsCandidate(column_num));
            continue;
        }
        if (max_lhs_ == 0) continue;

        MaximalSets maximal_sets = ComputeMaximalSets(agree_sets_vec, rhs_column_index);
        // There are no pairs where RHS attribute values are not equal, i. e. the column in the RHS
        // contains only a single unique value.
        assert(!maximal_sets.empty());  // should have been processed in the first if^

        // We don't invert the maximal sets like in the article since we can just treat them as if
        // they have been inverted later on.

        // Find what level 1 LHS candidates are.
        // See the comments in ComputeMaximalSetsIntersectionAndUnion.
        auto [fd_lhs_columns_complement, maximal_sets_intersection] =
                ComputeMaximalSetsUnionAndIntersection(maximal_sets);
        assert(!fd_lhs_columns_complement.test(rhs_column_index));
        assert(!maximal_sets_intersection.test(rhs_column_index));
        // Ideally, we would use agree sets without the RHS attribute in ComputeAgreeSets, but that
        // would need a more sophisticated copy that skips one bit, which boost::dynamic_bitset<>
        // doesn't provide. It might have a little bit of an effect on performance but said effect
        // could also be masked by the memory copying.
        // Instead, we have to dance a bit with fixing up the bitset on-the-fly to avoid dealing
        // with LHSs that have the RHS column, as they make for a trivial FD. For the higher levels,
        // supersets are generated by unions of LHS candidates from the previous level, so this also
        // ensures the RHS column will never show up in any of the further levels.
        // Without the dance, it would have been just the union.
        fd_lhs_columns_complement.set(rhs_column_index);  // max sets union + RHS
        // LHS size 1 is a special case to avoid useless checks when generating the next level.
        ReportFdLhssSingleColumn(fd_lhs_columns_complement, report_fd_lhs);
        NonFdLhsColumns non_fd_lhs_columns = util::BitsetToIndices<model::Index>(
                fd_lhs_columns_complement.reset(rhs_column_index) -= maximal_sets_intersection);
        if (non_fd_lhs_columns.size() < 2 || max_lhs_ == 1) continue;

        Level lhs_candidates = CreateSecondLevel(non_fd_lhs_columns, column_num);
        assert(!lhs_candidates.empty());
        do {
            FindFdLhss(lhs_candidates, maximal_sets, report_fd_lhs);
            if (lhs_candidates.size() < 2 || max_lhs_ == lhs_candidates.begin()->count()) break;
            lhs_candidates = ComputeNextLevelLhsCandidates(lhs_candidates);
        } while (!lhs_candidates.empty());
    }

    fd_view_ = std::make_shared<LhsMaskFdView>(table_header_, std::move(lhs_masks));
}
}  // namespace algos::fd
