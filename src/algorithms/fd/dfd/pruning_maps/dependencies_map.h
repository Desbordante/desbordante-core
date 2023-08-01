#pragma once

#include "vertical.h"
#include "pruning_map.h"

class DependenciesMap : public PruningMap {
public:
    explicit DependenciesMap(RelationalSchema const* schema);
    DependenciesMap() = default;

    std::unordered_set<Vertical> GetPrunedSubsets(std::unordered_set<Vertical> const& subsets) const;
    void AddNewDependency(Vertical const& node_to_add);
    bool CanBePruned(Vertical const& node) const;
};
