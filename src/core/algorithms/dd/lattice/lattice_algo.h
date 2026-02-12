#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/split/model/distance_position_list_index.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_index.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/builtin.h"

namespace algos::dd {

using DF = model::DF;
using DD = model::DD;
using DFConstraint = model::DFConstraint;
using Bitset = boost::dynamic_bitset<>;
using DFIdx = std::size_t;

class LatticeAlgorithm : public Algorithm {
private:
    double satisfaction_threshold_ = 1.0;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    unsigned num_rows_;
    model::ColumnIndex num_columns_;
    std::size_t tuple_pair_num_;

    std::vector<DistancePositionListIndex> plis_;
    std::vector<std::vector<std::vector<double>>> distances_;

    std::size_t df_num_;
    std::vector<std::vector<std::pair<std::size_t, std::size_t>>> base_intervals_;
    std::vector<std::size_t> column_for_df_;
    std::vector<std::pair<std::size_t, std::size_t>> column_partition_idxs_;
    std::vector<Bitset> base_partitions_;

    void CalculateBasePartitions();

    struct LatticeNode {
        Bitset df_;
        Bitset partition_;
        std::unordered_set<Bitset> dds_;

        LatticeNode() = default;

        LatticeNode(Bitset v, Bitset Fv, std::unordered_set<Bitset> dds)
            : df_(std::move(v)), partition_(std::move(Fv)), dds_(std::move(dds)) {}
    };

    std::vector<std::shared_ptr<LatticeNode>> current_level_;
    std::vector<std::shared_ptr<LatticeNode>> next_level_;
    void BuildFirstLevel();
    void BuildNextLevel();

    std::pair<double, double> FindRhs(Bitset const& df_partition, model::ColumnIndex col);

    struct DFTreeNode {
        std::optional<DFIdx> left_idx_, right_idx_;
        std::map<DFIdx, std::shared_ptr<DFTreeNode>> left_children_;

        DFTreeNode(std::optional<DFIdx> idx) : left_idx_(idx), right_idx_(idx) {}
    };

    struct DFTree {
        DFConstraint rhs_;
        std::shared_ptr<DFTreeNode> root_;

        DFTree(DFConstraint const& rhs)
            : rhs_(rhs), root_(std::make_shared<DFTreeNode>(std::nullopt)) {}
    };

    bool SameSubtrees(std::shared_ptr<DFTreeNode> subtree_a, std::shared_ptr<DFTreeNode> subtree_b);
    void Combine(std::shared_ptr<DFTreeNode> root, Bitset const& lhs);
    void CheckAndCombine(DFTree& tree, Bitset const& lhs);

    double CalculateDistance(model::ColumnIndex column_index,
                             std::pair<std::size_t, std::size_t> tuple_pair);
    void CalculateAllDistances();
};

}  // namespace algos::dd