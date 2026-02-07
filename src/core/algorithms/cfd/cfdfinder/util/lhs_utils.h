#pragma once

#include <list>

#include <boost/dynamic_bitset.hpp>

namespace algos::cfdfinder::utils {
using BitSet = boost::dynamic_bitset<>;

std::list<BitSet> GenerateLhsSubsets(BitSet const& lhs);
std::list<BitSet> GenerateLhsSupersets(BitSet const& lhs);

}  // namespace algos::cfdfinder::utils
