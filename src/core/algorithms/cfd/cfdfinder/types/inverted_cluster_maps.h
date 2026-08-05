#pragma once

#include <cstddef>
#include <map>
#include <string_view>
#include <vector>

namespace algos::cfdfinder {
using ClusterId = size_t;
using InvertedClusterMap = std::map<ClusterId, std::string_view>;
using InvertedClusterMaps = std::vector<InvertedClusterMap>;
}  // namespace algos::cfdfinder
