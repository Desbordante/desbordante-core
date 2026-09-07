#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

#include "core/algorithms/fd/dfd/lattice_observations/lattice_observations.h"

class PruningMap : public std::unordered_map<boost::dynamic_bitset<>,
                                             std::unordered_set<boost::dynamic_bitset<>>> {
public:
    PruningMap(std::size_t num_columns);
    PruningMap() = default;

    void Rebalance();
    void RebalanceGroup(boost::dynamic_bitset<> const& key);
};
