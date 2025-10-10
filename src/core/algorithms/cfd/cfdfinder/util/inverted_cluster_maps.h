#pragma once

#include <map>
#include <string>
#include <vector>

namespace algos::cfdfinder {
using InvertedClusterMap = std::map<size_t, std::string>;
using InvertedClusterMaps = std::vector<InvertedClusterMap>;
}  // namespace algos::cfdfinder