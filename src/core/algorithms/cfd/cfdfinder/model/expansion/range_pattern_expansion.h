#pragma once

#include <list>
#include <memory>
#include <vector>

#include "algorithms/cfd/cfdfinder/model/expansion/expansion_strategy.h"
#include "algorithms/cfd/cfdfinder/types/inverted_cluster_maps.h"

namespace algos::cfdfinder {

class RangePatternExpansion : public ExpansionStrategy {
private:
    using SortedClustersId = std::vector<ClusterId>;
    using SortedClustersIdPtr = std::shared_ptr<SortedClustersId>;
    std::vector<SortedClustersIdPtr> sorted_clusters_ids_;

public:
    explicit RangePatternExpansion(InvertedClusterMaps const& inverted_cluster_maps);
    Pattern GenerateNullPattern(BitSet const& attributes) const override;
    std::list<Pattern> GetChildPatterns(Pattern const& current_pattern) const override;
};

}  // namespace algos::cfdfinder
