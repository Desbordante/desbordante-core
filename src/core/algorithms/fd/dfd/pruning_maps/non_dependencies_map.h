#pragma once

#include <unordered_set>  // for unordered_set

#include "model/table/vertical.h"  // for Vertical
#include "pruning_map.h"           // for PruningMap

class RelationalSchema;

class NonDependenciesMap : public PruningMap {
public:
    explicit NonDependenciesMap(RelationalSchema const* schema);
    NonDependenciesMap() = default;

    std::unordered_set<Vertical> GetPrunedSupersets(
            std::unordered_set<Vertical> const& supersets) const;
    void AddNewNonDependency(Vertical const& node_to_add);
    bool CanBePruned(Vertical const& node) const;
};
