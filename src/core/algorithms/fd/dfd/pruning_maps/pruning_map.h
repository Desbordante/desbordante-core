#pragma once

#include <unordered_map>
#include <unordered_set>

#include "core/algorithms/fd/dfd/lattice_observations/lattice_observations.h"

class PruningMap : public std::unordered_map<boost::dynamic_bitset<>,
                                             std::unordered_set<boost::dynamic_bitset<>>> {
public:
    PruningMap(RelationalSchema const* schema);
    PruningMap() = default;

    void Rebalance();
    void RebalanceGroup(boost::dynamic_bitset<> const& key);
};
