#pragma once

#include <list>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/cfd/cfdfinder/candidate.h"
#include "algorithms/cfd/cfdfinder/util/bitset_util.h"

namespace algos::cfdfinder::util {
using BitSet = boost::dynamic_bitset<>;

std::list<BitSet> GenerateLhsSubsets(BitSet const& lhs);
std::list<BitSet> GenerateLhsSupersets(BitSet const& lhs);

}  // namespace algos::cfdfinder::util
