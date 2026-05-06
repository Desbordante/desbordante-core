#pragma once

#include <cstddef>
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/lattice/bitset.h"
#include "core/algorithms/dd/split/model/distance_position_list_index.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_index.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/builtin.h"

namespace algos::dd {

using DFConstraint = model::DFConstraint;
using DFStringConstraint = model::DFStringConstraint;
using DFStringList = std::list<model::DFStringConstraint>;
using DDString = model::DDString;
using DFIdx = int;
using DDSet = std::shared_ptr<std::unordered_set<Bitset, BitsetHash>>;

class LatticeAlgorithm : public Algorithm {
private:
    config::InputTable input_table_;

    double satisfaction_threshold_;
    double support_threshold_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    unsigned num_rows_;
    model::ColumnIndex num_columns_;
    std::size_t tuple_pair_num_;
    std::size_t tuple_pair_threshold_;

    std::vector<model::TypeId> type_ids_;
    config::InputTable difference_table_;
    std::unique_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation_;

    std::vector<DFConstraint> min_max_dif_;
    std::vector<DistancePositionListIndex> plis_;
    std::vector<std::vector<std::vector<double>>> distances_;
    std::vector<std::vector<DFConstraint>> index_search_spaces_;
    std::vector<model::ColumnIndex> non_empty_cols_;
    std::list<model::DDString> DDs_;

    std::size_t df_num_;
    std::vector<std::size_t> df_column_;
    std::vector<std::size_t> column_begin_idx_;
    std::vector<std::vector<Bitset>> base_partitions_;

    void RegisterOptions();
    void SetLimits();
    void CheckTypes();
    void ParseDifferenceTable();

    void ResetState() final {
        DDs_.clear();
        trees_.clear();

        min_max_dif_.clear();
        plis_.clear();
        distances_.clear();
        index_search_spaces_.clear();
        non_empty_cols_.clear();

        current_level_.clear();
        next_level_.clear();

        df_column_.clear();
        column_begin_idx_.clear();
        base_partitions_.clear();
    }

    double CalculateDistance(model::ColumnIndex column_index,
                             std::pair<std::size_t, std::size_t> tuple_pair);
    void CalculateAllDistances();
    std::vector<DFConstraint> IndexSearchSpace(model::ColumnIndex index);
    void CalculateIndexSearchSpaces();
    [[gnu::always_inline, gnu::hot]] bool CheckDFConstraint(
            DFConstraint const& dif_constraint, model::ColumnIndex column_index,
            std::pair<std::size_t, std::size_t> tuple_pair);
    void CalculateTuplePairs();

    struct LatticeNode {
        Bitset df_;
        Bitset partition_;
        DDSet dds_;

        LatticeNode()
            : df_(std::move(make_bitset(0))),
              partition_(std::move(make_bitset(0))),
              dds_(std::make_shared<std::unordered_set<Bitset, BitsetHash>>()) {}

        LatticeNode(Bitset v, Bitset Fv, DDSet dds)
            : df_(std::move(v)), partition_(std::move(Fv)), dds_(dds) {}
    };

    std::vector<std::unique_ptr<LatticeNode>> current_level_;
    std::vector<std::unique_ptr<LatticeNode>> next_level_;
    void BuildFirstLevel();
    void BuildNextLevel();

    void FindRhs(Bitset const& df_partition, model::ColumnIndex col, Bitset& col_intervals);

    struct DFTreeNode {
        std::optional<DFIdx> left_idx_, right_idx_;
        std::map<DFIdx, std::unique_ptr<DFTreeNode>> left_children_;

        DFTreeNode() : left_idx_(std::nullopt), right_idx_(std::nullopt) {}

        DFTreeNode(DFIdx idx) : left_idx_(idx), right_idx_(idx) {}
    };

    bool SameSubtrees(DFTreeNode* subtree_a, DFTreeNode* subtree_b);
    void Combine(DFTreeNode* root, Bitset const& lhs);
    void CheckAndCombine(DFTreeNode* root, Bitset const& lhs);

    void minDD();

    std::unordered_map<Bitset, std::unique_ptr<DFTreeNode>, BitsetHash> trees_;

    DFStringConstraint MakeDF(Bitset const& bitset_df);
    DFStringConstraint MakeDF(DFIdx left_idx, DFIdx right_idx);
    void TreeNodeDFS(DFTreeNode* node, DFStringList const& rhs, DFStringList& lhs);
    void CollectPaths();

    void PrintResults();

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    LatticeAlgorithm();
    std::list<DDString> GetDDStringList() const;
};

}  // namespace algos::dd