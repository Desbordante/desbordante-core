#pragma once

#include <unordered_map>
#include <unordered_set>

#include "../lattice_observations/lattice_observations.h"
#include "model/table/vertical.h"

class PruningMap : public std::unordered_map<Vertical, std::unordered_set<Vertical>> {
public:
    PruningMap(RelationalSchema const* schema);
    PruningMap() = default;

    void Rebalance();
    void RebalanceGroup(Vertical const& key);
};
