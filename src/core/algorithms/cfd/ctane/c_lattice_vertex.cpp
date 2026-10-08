#include "core/algorithms/cfd/ctane/c_lattice_vertex.h"

#include <algorithm>
#include <iterator>
#include <utility>

using boost::dynamic_bitset;

CLatticeVertex::CLatticeVertex(algos::cfd::CFDRelationData const* relation,
                               algos::cfd::TuplePattern tuple_pattern)
    : relation_(relation), tuple_pattern_(std::move(tuple_pattern)) {}

algos::cfd::CFDRelationData const* CLatticeVertex::GetRelation() const {
    return relation_;
}

algos::cfd::TuplePattern const& CLatticeVertex::GetTuplePattern() const {
    return tuple_pattern_;
}

auto const& CLatticeVertex::GetPatternValues() const {
    return GetTuplePattern().GetPatternValues();
}

algos::cfd::PartitionTIdList const* CLatticeVertex::GetPositionListIndex() const {
    return partition_ ? &*partition_ : nullptr;
}

std::vector<CLatticeVertex const*>& CLatticeVertex::GetParents() {
    return parents_;
}

std::vector<CLatticeVertex const*> const& CLatticeVertex::GetParents() const {
    return parents_;
}

algos::cfd::Itemset& CLatticeVertex::GetRhsCandidates() {
    return rhs_candidates_;
}

algos::cfd::Itemset const& CLatticeVertex::GetRhsCandidates() const {
    return rhs_candidates_;
}

void CLatticeVertex::SetRhsCandidates(algos::cfd::Itemset candidates) {
    rhs_candidates_ = std::move(candidates);
}

void CLatticeVertex::SetPositionListIndex(algos::cfd::PartitionTIdList partition) {
    partition_ = std::move(partition);
}

bool CLatticeVertex::operator>(CLatticeVertex const& rhs) const {
    if (GetColumnIndices().count() != rhs.GetColumnIndices().count()) {
        return GetColumnIndices().count() > rhs.GetColumnIndices().count();
    }

    auto const& indices = GetColumnIndices();
    auto const& rhs_indices = rhs.GetColumnIndices();

    for (auto [idx, rhs_idx] = std::pair(indices.find_first(), rhs_indices.find_first());
         idx != dynamic_bitset<>::npos || rhs_idx != dynamic_bitset<>::npos;
         std::tie(idx, rhs_idx) =
                 std::pair(indices.find_next(idx), rhs_indices.find_next(rhs_idx))) {
        auto result = (long)idx - (long)rhs_idx;
        if (result) {
            return (result > 0);
        }
    }

    if (rhs.GetTuplePattern().IsMoreGeneralThan(GetTuplePattern())) {
        return true;
    }

    return GetPatternValues() > rhs.GetPatternValues();
}

bool CLatticeVertex::operator<(CLatticeVertex const& rhs) const {
    return rhs > *this;
}

bool CLatticeVertex::Comparator(CLatticeVertex* v_1, CLatticeVertex* v_2) {
    return *v_1 < *v_2;
}

bool CLatticeVertex::ComesBeforeAndSharePrefixWith(CLatticeVertex const& rhs) const {
    auto const& indices = GetColumnIndices();
    auto const& rhs_indices = rhs.GetColumnIndices();

    auto index = indices.find_first();
    auto rhs_index = rhs_indices.find_first();

    auto arity = indices.count();
    for (unsigned long i = 0; i < arity - 1; i++) {
        if (index != rhs_index ||
            GetPatternValues().at(index) != rhs.GetPatternValues().at(rhs_index)) {
            return false;
        }
        index = indices.find_next(index);
        rhs_index = rhs_indices.find_next(rhs_index);
    }
    return index < rhs_index;
}

bool CLatticeVertex::RhsCandidateComparator(algos::cfd::Item lhs, algos::cfd::Item rhs) {
    return lhs < rhs;
}

algos::cfd::Itemset CLatticeVertex::IntersectRhsCandidates(algos::cfd::Itemset const& lhs,
                                                           algos::cfd::Itemset const& rhs) {
    algos::cfd::Itemset intersection;
    std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                          std::back_inserter(intersection), RhsCandidateComparator);
    return intersection;
}
