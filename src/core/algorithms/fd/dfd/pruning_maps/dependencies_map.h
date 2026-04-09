#pragma once

#include <unordered_set>

#include "core/algorithms/fd/dfd/pruning_maps/pruning_map.h"
#include "core/model/table/relational_schema.h"
#include "core/model/table/vertical.h"

class DependenciesMap : public PruningMap {
public:
    explicit DependenciesMap(RelationalSchema const* schema);
    DependenciesMap() = default;

    std::unordered_set<Vertical> GetPrunedSubsets(
            std::unordered_set<Vertical> const& subsets) const;
    void AddNewDependency(Vertical const& node_to_add);
    bool CanBePruned(Vertical const& node) const;
};
