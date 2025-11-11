#pragma once

#include <map>
#include <string>
#include <vector>

namespace algos::cfdfinder {
using ClusterId = size_t;
using AttributeValue = std::string;
using InvertedClusterMap = std::map<ClusterId, AttributeValue>;
using InvertedClusterMaps = std::vector<InvertedClusterMap>;
}  // namespace algos::cfdfinder