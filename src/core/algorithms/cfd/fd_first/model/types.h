#pragma once

#include <utility>
#include <vector>

#include <boost/unordered_map.hpp>

#include "core/algorithms/cfd/model/cfd_types.h"

// see algorithms/cfd/fd_first/LICENSE

namespace algos::cfd {

using PartitionList = std::vector<std::pair<Itemset, std::vector<unsigned>>>;
using RhsesPair2DList = std::vector<std::vector<std::pair<int, int>>>;
using RhsesPairList = std::vector<std::pair<int, int>>;
using RuleIxs = boost::unordered_map<Itemset, unsigned>;

}  // namespace algos::cfd
