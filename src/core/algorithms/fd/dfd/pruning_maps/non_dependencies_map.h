#pragma once

#include <cstddef>

#include "core/algorithms/fd/dfd/pruning_maps/pruning_map.h"

class NonDependenciesMap : public PruningMap {
public:
    explicit NonDependenciesMap(std::size_t num_columns);
    NonDependenciesMap() = default;

    std::unordered_set<boost::dynamic_bitset<>> GetPrunedSupersets(
            std::unordered_set<boost::dynamic_bitset<>> const& supersets) const;
    void AddNewNonDependency(boost::dynamic_bitset<> const& node_to_add);
    bool CanBePruned(boost::dynamic_bitset<> const& node) const;
};
