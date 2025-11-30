#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "algorithms/cfd/cfdfinder/types/cluster.h"

namespace algos::cfdfinder {
using AttributeValue = std::string;
using ClusterMap = std::unordered_map<AttributeValue, Cluster>;
using ClusterMaps = std::vector<ClusterMap>;
}  // namespace algos::cfdfinder
