#pragma once

#include <vector>

#include "cfd/cfdfinder/types/inverted_cluster_maps.h"
#include "expansion_strategy.h"

namespace algos::cfdfinder {

class RangePatternExpansion : public ExpansionStrategy {
private:
    using SortedClustersId = std::vector<ClusterId>;
    using SortedClustersIdPtr = std::shared_ptr<SortedClustersId>;
    std::vector<SortedClustersIdPtr> sorted_clusters_ids_;

public:
    RangePatternExpansion(InvertedClusterMaps const& inverted_cluster_maps);
    Pattern GenerateNullPattern(BitSet const& attributes) override;
    std::list<Pattern> GetChildPatterns(Pattern const& current_pattern) override;
};

}  // namespace algos::cfdfinder