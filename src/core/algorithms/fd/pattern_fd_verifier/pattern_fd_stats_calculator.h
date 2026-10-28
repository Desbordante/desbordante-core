#pragma once

#include <climits>
#include <map>
#include <unordered_set>
#include <utility>

#include "core/algorithms/fd/pattern_fd_verifier/highlight.h"
#include "core/algorithms/fd/pattern_fd_verifier/model/pattern_info.h"
#include "core/algorithms/fd/pattern_fd_verifier/model/tokenizer.h"
#include "core/config/indices/type.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/table/position_list_index.h"

namespace algos::pattern_fd {

using ClusterKey = std::vector<std::vector<std::string>>;
using RowClusters = std::map<ClusterKey, std::vector<size_t>>;

class PatternFDStatsCalculator {
private:
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    PatternsTable patterns_table_;
    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;

    Tokenizer tokenizer_;
    std::vector<std::unordered_set<size_t>> lhs_matched_rows_;  // rows for each pattern row

    std::vector<Highlight> highlights_;
    int pattern_fd_coverage_ = 0;
    int min_pattern_inclusion_ = INT_MAX;
    double max_rhs_deviation_ = 0.0;
    size_t num_error_rows_ = 0;

    void FindLhsMatches();

    void CompareRhsValues();

    std::unordered_set<size_t> FindLhsMatchesForRowPattern(
            std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns);

    bool Matches(config::IndexType column_index, std::shared_ptr<PatternInfo> const& pattern,
                 std::string const& value) const;

    RowClusters BuildLhsClusters(
            std::unordered_set<size_t> const& matched_rows,
            std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns);

    size_t CheckClustersForDeviations(
            RowClusters const& lhs_clusters,
            std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns,
            std::unordered_set<size_t>& error_rows);

public:
    PatternFDStatsCalculator(std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation)
        : typed_relation_(std::move(typed_relation)), tokenizer_(typed_relation_) {}

    void SetParams(PatternsTable const& patterns_table, config::IndicesType const& lhs_indices,
                   config::IndicesType const& rhs_indices) {
        this->patterns_table_ = patterns_table;
        this->lhs_indices_ = lhs_indices;
        this->rhs_indices_ = rhs_indices;
        for (auto& lhs_index : lhs_indices) {
            tokenizer_.PrepareColumn(lhs_index);
        }
        for (auto& rhs_index : rhs_indices) {
            tokenizer_.PrepareColumn(rhs_index);
        }
    }

    void CalculateStatistics();

    void ResetState();

    size_t GetNumErrorClusters() const {
        return highlights_.size();
    }

    size_t GetNumErrorRows() const {
        return num_error_rows_;
    }

    std::vector<pattern_fd::Highlight> const& GetClusters() const {
        return highlights_;
    }

    int GetPatternFDCoverage() const {
        return pattern_fd_coverage_;
    }

    int GetMinPatternInclusion() const {
        return min_pattern_inclusion_;
    }

    double GetMaxRhsDeviation() const {
        return max_rhs_deviation_;
    }
};

}  // namespace algos::pattern_fd
