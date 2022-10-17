#pragma once

#include <unordered_set>
#include <unordered_map>

#include "vertical.h"
#include "dfd/lattice_observations/lattice_observations.h"

class PruningMap : public std::unordered_map<Vertical, std::unordered_set<Vertical>> {
public:
    PruningMap(RelationalSchema const* schema);
    PruningMap() = default;

    void Rebalance();
    void RebalanceGroup(Vertical const& key);
};
