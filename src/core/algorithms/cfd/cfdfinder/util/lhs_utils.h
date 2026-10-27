#pragma once

#include <list>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfdfinder/types/cluster.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder::utils {
using BitSet = boost::dynamic_bitset<>;

std::list<BitSet> GenerateLhsSubsets(BitSet const& lhs);
std::list<BitSet> GenerateLhsSupersets(BitSet const& lhs);
std::vector<Cluster> EnrichPLI(model::PLI const* pli, size_t num_tuples);
}  // namespace algos::cfdfinder::utils
