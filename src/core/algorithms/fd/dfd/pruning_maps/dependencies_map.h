#pragma once

#include <cstddef>

#include "core/algorithms/fd/dfd/pruning_maps/pruning_map.h"

class DependenciesMap : public PruningMap {
public:
    explicit DependenciesMap(std::size_t num_columns);
    DependenciesMap() = default;

    std::unordered_set<boost::dynamic_bitset<>> GetPrunedSubsets(
            std::unordered_set<boost::dynamic_bitset<>> const& subsets) const;
    void AddNewDependency(boost::dynamic_bitset<> const& node_to_add);
    bool CanBePruned(boost::dynamic_bitset<> const& node) const;
};
