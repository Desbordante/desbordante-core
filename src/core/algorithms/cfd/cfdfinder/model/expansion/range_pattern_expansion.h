#pragma once

#include <cstddef>
#include <list>
#include <memory>
#include <vector>

#include "core/algorithms/cfd/cfdfinder/model/expansion/expansion_strategy.h"
#include "core/algorithms/cfd/cfdfinder/types/inverted_cluster_maps.h"

namespace algos::cfdfinder {

class RangeEntry;

class RangePatternExpansion : public ExpansionStrategy {
private:
    using SortedClustersId = std::vector<ClusterId>;
    using SortedClustersIdPtr = std::shared_ptr<SortedClustersId const>;
    using RangeEntryPtr = std::shared_ptr<RangeEntry>;

    struct RangeCover {
        size_t support;
        boost::dynamic_bitset<> cover_mask;
    };

    std::vector<SortedClustersIdPtr> sorted_clusters_ids_;

    Entries GenerateNullEntries(BitSet const& attributes) const override;
    std::vector<RangeEntryPtr> GenerateReplacements(RangeEntry const& range_entry) const;
    std::vector<RangeEntryPtr> FilterValidReplacements(
            Entries& entries, size_t replaced_index, std::vector<RangeEntryPtr>&& replacements,
            std::shared_ptr<PruningStrategy> pruning_strategy) const;
    RangeCover CalculateCover(RangeEntry const& entry,
                              std::vector<size_t> const& cluster_representatives,
                              Cover const& cover) const;

public:
    RangePatternExpansion(InvertedClusterMaps const& inverted_cluster_maps,
                          RowsPtr&& compressed_records);

    void Expand(Pattern&& parent_pattern, Frontier& frontier, Row const& inverted_pli_rhs,
                std::shared_ptr<PruningStrategy> pruning_strategy) const override;
};

}  // namespace algos::cfdfinder
