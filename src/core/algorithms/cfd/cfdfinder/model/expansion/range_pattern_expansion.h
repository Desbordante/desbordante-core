#pragma once

#include <list>
#include <map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "cfd/cfdfinder/util/inverted_cluster_maps.h"
#include "expansion_strategy.h"

namespace algos::cfdfinder {

class RangePatternExpansion : public ExpansionStrategy {
private:
    std::unordered_map<size_t, std::vector<size_t>> sorted_clusters_ids_;

public:
    RangePatternExpansion(InvertedClusterMaps const& inverted_cluster_maps);
    Pattern GenerateNullPattern(boost::dynamic_bitset<> const& attributes) override;
    std::list<Pattern> GetChildPatterns(Pattern const& current_pattern) override;
};

}  // namespace algos::cfdfinder