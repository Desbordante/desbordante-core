#pragma once

#include <unordered_map>  // for unordered_map
#include <unordered_set>  // for unordered_set

#include "custom_hashes.h"         // for hash
#include "model/table/vertical.h"  // for Vertical

class RelationalSchema;

class PruningMap : public std::unordered_map<Vertical, std::unordered_set<Vertical>> {
public:
    PruningMap(RelationalSchema const* schema);
    PruningMap() = default;

    void Rebalance();
    void RebalanceGroup(Vertical const& key);
};
