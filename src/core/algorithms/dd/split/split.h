#pragma once

#include <cstddef>
#include <filesystem>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/dd/dd.h"
#include "algorithms/dd/split/model/distance_position_list_index.h"
#include "config/tabular_data/input_table_type.h"
#include "enums.h"
#include "model/table/column_index.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "model/types/builtin.h"

namespace algos::dd {

using DF = model::DF;
using DD = model::DD;
using DFConstraint = model::DFConstraint;

class Split : public Algorithm {
private:
    config::InputTable input_table_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    unsigned num_rows_;
    model::ColumnIndex num_columns_;
    std::vector<model::ColumnIndex> non_empty_cols_;
    std::size_t tuple_pair_num_;

    std::vector<model::TypeId> type_ids_;

    config::InputTable difference_table_;
    std::unique_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation_;

    Reduce const reduce_method_ = Reduce::IEHybrid;  // currently, the fastest method
    unsigned const num_dfs_per_column_ = 5;

    std::vector<DistancePositionListIndex> plis_;
    std::vector<DFConstraint> min_max_dif_;
    std::vector<std::vector<std::vector<double>>> distances_;
    std::vector<std::pair<std::size_t, std::size_t>> tuple_pairs_;
    std::vector<std::vector<DFConstraint>> index_search_spaces_;
    std::list<DD> dd_collection_;

    void RegisterOptions();
    void SetLimits();
    void CheckTypes();
    void ParseDifferenceTable();

    void ResetState() final {
        dd_collection_.clear();
        tuple_pairs_.clear();
        non_empty_cols_.clear();
        index_search_spaces_.clear();
        distances_.clear();
    }

    double CalculateDistance(model::ColumnIndex column_index,
                             std::pair<std::size_t, std::size_t> tuple_pair);
    void InsertDistance(model::ColumnIndex column_index, std::size_t first_index,
                        std::size_t second_index, double& min_dif, double& max_dif);
    [[gnu::always_inline, gnu::hot]] bool CheckDFConstraint(
            DFConstraint const& dif_constraint, model::ColumnIndex column_index,
            std::pair<std::size_t, std::size_t> tuple_pair);
    [[gnu::always_inline, gnu::hot]] bool CheckDF(DF const& dep,
                                                  std::pair<std::size_t, std::size_t> tuple_pair);
    bool VerifyDD(DF const& lhs, DF const& rhs);
    void CalculateIndexSearchSpaces();
    void CalculateTuplePairs();
    void CalculateAllDistances();
    bool IsFeasible(DF const& d);
    std::vector<DF> SearchSpace(std::vector<model::ColumnIndex>& indices);
    std::vector<DF> SearchSpace(model::ColumnIndex index);
    std::vector<DFConstraint> IndexSearchSpace(model::ColumnIndex index);
    bool Subsume(DF const& df1, DF const& df2);
    std::vector<DF> DoNegativePruning(std::vector<DF> const& search, DF const& last_df);
    std::pair<std::vector<DF>, std::vector<DF>> NegativeSplit(std::vector<DF> const& search,
                                                              DF const& last_df);
    std::vector<DF> DoPositivePruning(std::vector<DF> const& search, DF const& first_df);
    std::pair<std::vector<DF>, std::vector<DF>> PositiveSplit(std::vector<DF> const& search,
                                                              DF const& first_df);
    std::list<DD> MergeReducedResults(std::list<DD> const& base_dds,
                                      std::list<DD> const& dds_to_merge);
    std::list<DD> NegativePruningReduce(DF const& rhs, std::vector<DF> const& search,
                                        unsigned& cnt);
    std::list<DD> HybridPruningReduce(DF const& rhs, std::vector<DF> const& search, unsigned& cnt);
    std::list<DD> InstanceExclusionReduce(std::vector<std::size_t> const& tuple_pair_indices,
                                          std::vector<DF> const& search, DF const& rhs,
                                          unsigned& cnt);
    unsigned ReduceDDs(auto const& start_time);
    unsigned RemoveRedundantDDs();
    unsigned RemoveTransitiveDDs();
    model::DDString DDToDDString(DD const& dd) const;
    void PrintResults();

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    Split();
    std::list<DD> const& GetDDs() const;
    std::vector<model::DFConstraint> const& GetMinMaxDif() const;
    std::list<model::DDString> GetDDStringList() const;
};

}  // namespace algos::dd
