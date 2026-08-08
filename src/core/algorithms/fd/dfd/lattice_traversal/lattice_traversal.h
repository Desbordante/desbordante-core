#pragma once

#include <random>
#include <stack>

#include "core/algorithms/fd/dfd/column_order/column_order.h"
#include "core/algorithms/fd/dfd/lattice_observations/lattice_observations.h"
#include "core/algorithms/fd/dfd/partition_storage/partition_storage.h"
#include "core/algorithms/fd/dfd/pruning_maps/dependencies_map.h"
#include "core/algorithms/fd/dfd/pruning_maps/non_dependencies_map.h"
#include "core/model/index.h"

class LatticeTraversal {
private:
    model::Index const rhs_index_;

    std::unordered_set<boost::dynamic_bitset<>> minimal_deps_;
    std::unordered_set<boost::dynamic_bitset<>> maximal_non_deps_;
    DependenciesMap dependencies_map_;
    NonDependenciesMap non_dependencies_map_;
    LatticeObservations observations_;
    std::stack<boost::dynamic_bitset<>> trace_;
    ColumnOrder const column_order_;

    std::vector<boost::dynamic_bitset<>> const& unique_columns_;
    ColumnLayoutRelationData const* const relation_;
    PartitionStorage* const partition_storage_;

    std::random_device rd_;
    std::mt19937 gen_;

    bool InferCategory(boost::dynamic_bitset<> const& node, unsigned int rhs_index);
    boost::dynamic_bitset<> PickNextNode(boost::dynamic_bitset<> const& node,
                                         unsigned int rhs_index);
    std::stack<boost::dynamic_bitset<>> GenerateNextSeeds(model::Index const current_rhs);

    std::list<boost::dynamic_bitset<>> Minimize(
            std::unordered_set<boost::dynamic_bitset<>> const& node_list) const;
    boost::dynamic_bitset<> const& TakeRandom(
            std::unordered_set<boost::dynamic_bitset<>>& node_set);
    static void SubtractSets(std::unordered_set<boost::dynamic_bitset<>>& set,
                             std::unordered_set<boost::dynamic_bitset<>> const& set_to_subtract);

public:
    LatticeTraversal(model::Index rhs_index, ColumnLayoutRelationData const* const relation,
                     std::vector<boost::dynamic_bitset<>> const& unique_verticals,
                     PartitionStorage* const partition_storage);

    std::unordered_set<boost::dynamic_bitset<>> FindLHSs();
};
