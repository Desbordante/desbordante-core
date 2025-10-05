#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "cluster.h"

namespace algos::cfdfinder {
using ClusterMap = std::unordered_map<std::string, Cluster>;
using ClusterMaps = std::vector<ClusterMap>;

}  // namespace algos::cfdfinder