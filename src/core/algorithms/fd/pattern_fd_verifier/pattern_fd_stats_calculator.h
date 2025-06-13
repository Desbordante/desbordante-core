#pragma once

#include <limits>

#include "algorithms/fd/pattern_fd_verifier/highlight.h"
#include "algorithms/fd/pattern_fd_verifier/model/pattern_info.h"
#include "algorithms/fd/pattern_fd_verifier/model/tokenizer.h"
#include "config/indices/type.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "model/table/position_list_index.h"

namespace algos::pattern_fd {

class PatternFDStatsCalculator {
private:
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    PatternsTable patterns_table_;
    config::IndicesType lhs_indices_;
    config::IndexType rhs_index_;

    Tokenizer tokenizer_;
    std::unique_ptr<std::unordered_map<config::IndexType,
                                       std::unordered_map<TokenPatternInfo, std::vector<size_t>>>>
            tokens_map_;  // maps of tokens for each column

    std::vector<std::unordered_set<config::IndexType>>
            lhs_matched_rows_;  // rows matched for each LHS index

    std::vector<model::PLI::Cluster> clusters_;
    std::vector<Highlight> highlights_;
    int pattern_fd_coverage_ = 0;
    int min_pattern_inclusion_ = INT_MAX;
    double max_rhs_deviation_ = 0.0;
    size_t num_error_rows_ = 0;

    void FindLhsMatches();

    void CompareRhsValues();

public:
    PatternFDStatsCalculator(std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation,
                             PatternsTable const& patterns_table,
                             config::IndicesType const& lhs_indices,
                             config::IndexType const& rhs_index)
        : typed_relation_(std::move(typed_relation)),
          patterns_table_(patterns_table),
          lhs_indices_(lhs_indices),
          rhs_index_(rhs_index),
          tokenizer_(typed_relation_) {
        tokenizer_.TokenizeColumns();
        tokens_map_ = std::make_unique<std::unordered_map<
                config::IndexType, std::unordered_map<TokenPatternInfo, std::vector<size_t>>>>(
                tokenizer_.GetTokensMap());
    }

    PatternFDStatsCalculator() = default;

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
