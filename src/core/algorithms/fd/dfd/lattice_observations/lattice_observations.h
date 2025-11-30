#pragma once

#include <unordered_map>

#include "core/algorithms/fd/dfd/column_order/column_order.h"
#include "core/algorithms/fd/dfd/node_category.h"
#include "core/model/table/vertical.h"
#include "core/util/custom_hashes.h"

class LatticeObservations : public std::unordered_map<Vertical, NodeCategory> {
public:
    bool IsCandidate(Vertical const& node) const;

    bool IsVisited(Vertical const& node) const {
        return this->find(node) != this->end();
    }

    NodeCategory UpdateDependencyCategory(Vertical const& node);
    NodeCategory UpdateNonDependencyCategory(Vertical const& node, unsigned int rhs_index);

    std::unordered_set<Vertical> GetUncheckedSubsets(Vertical const& node,
                                                     ColumnOrder const&) const;
    std::unordered_set<Vertical> GetUncheckedSupersets(Vertical const& node, unsigned int rhs_index,
                                                       ColumnOrder const&) const;
};
