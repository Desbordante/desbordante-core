#pragma once

#include <unordered_map>

#include "vertical.h"
#include "../src/custom/custom_hashes.h"
#include "dfd/column_order/column_order.h"
#include "dfd/node_category.h"

class LatticeObservations : public std::unordered_map<Vertical, NodeCategory> {
public:
    bool IsCandidate(Vertical const& node) const;
    bool IsVisited(Vertical const& node) const { return this->find(node) != this->end(); }

    NodeCategory UpdateDependencyCategory(Vertical const& node);
    NodeCategory UpdateNonDependencyCategory(Vertical const& node, unsigned int rhs_index);

    std::unordered_set<Vertical> GetUncheckedSubsets(const Vertical& node,
                                                     ColumnOrder const&) const;
    std::unordered_set<Vertical> GetUncheckedSupersets(const Vertical& node, unsigned int rhs_index,
                                                       ColumnOrder const&) const;
};
