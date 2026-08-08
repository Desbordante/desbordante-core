#pragma once

#include "core/algorithms/fd/dfd/pruning_maps/pruning_map.h"

class NonDependenciesMap : public PruningMap {
public:
    explicit NonDependenciesMap(RelationalSchema const* schema);
    NonDependenciesMap() = default;

    std::unordered_set<boost::dynamic_bitset<>> GetPrunedSupersets(
            std::unordered_set<boost::dynamic_bitset<>> const& supersets) const;
    void AddNewNonDependency(boost::dynamic_bitset<> const& node_to_add);
    bool CanBePruned(boost::dynamic_bitset<> const& node) const;
};
