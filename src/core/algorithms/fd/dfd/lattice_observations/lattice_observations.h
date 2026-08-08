#pragma once

#include <unordered_map>

#include "core/algorithms/fd/dfd/column_order/column_order.h"
#include "core/algorithms/fd/dfd/node_category.h"

class LatticeObservations : public std::unordered_map<boost::dynamic_bitset<>, NodeCategory> {
public:
    bool IsCandidate(boost::dynamic_bitset<> const& node) const;

    bool IsVisited(boost::dynamic_bitset<> const& node) const {
        return this->find(node) != this->end();
    }

    NodeCategory UpdateDependencyCategory(boost::dynamic_bitset<> const& node);
    NodeCategory UpdateNonDependencyCategory(boost::dynamic_bitset<> const& node,
                                             unsigned int rhs_index);

    std::unordered_set<boost::dynamic_bitset<>> GetUncheckedSubsets(
            boost::dynamic_bitset<> const& node, ColumnOrder const&) const;
    std::unordered_set<boost::dynamic_bitset<>> GetUncheckedSupersets(
            boost::dynamic_bitset<> const& node, unsigned int rhs_index, ColumnOrder const&) const;
};
