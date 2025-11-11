#pragma once

#include <list>
#include <set>

#include "algorithms/cfd/cfdfinder/candidate.h"
#include "types/bitset.h"

namespace algos::cfdfinder::util {
std::list<BitSet> GenerateLhsSubsets(BitSet const& lhs);
std::list<BitSet> GenerateLhsSupersets(BitSet const& lhs);
void AddLhsSubsets(Candidate const& candidate, std::set<Candidate>& level);
}  // namespace algos::cfdfinder::util