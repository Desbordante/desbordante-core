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
    config::IndicesType rhs_indices_;

    Tokenizer tokenizer_;
    std::unordered_map<config::IndexType,
                       std::unordered_map<TokenPatternInfo, std::vector<size_t>>> const*
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

    std::unordered_set<config::IndexType> FindLhsMatchesForRowPattern(
            std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns);

    std::unordered_set<config::IndexType> FindRegexMatches(
            config::IndexType lhs_index,
            std::shared_ptr<RegexPatternInfo> const& regex_pattern_info);

    std::map<std::vector<std::string>, std::vector<config::IndexType>> BuildLhsClusters(
            std::unordered_set<config::IndexType> const& matched_rows,
            std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns);

    size_t CheckClustersForDeviations(
            std::map<std::vector<std::string>, std::vector<config::IndexType>> const& lhs_clusters,
            std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns);

    std::vector<size_t> FindDeviationsByPatternCheck(
            std::vector<config::IndexType> const& cluster_rows,
            std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns);

    std::vector<size_t> FindDeviationsByComparison(
            std::vector<config::IndexType> const& cluster_rows,
            std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns);

public:
    PatternFDStatsCalculator(std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation)
        : typed_relation_(typed_relation), tokenizer_(typed_relation_) {
        tokens_map_ = &tokenizer_.GetTokensMap();
    }

    void SetParams(PatternsTable const& patterns_table, config::IndicesType const& lhs_indices,
                   config::IndicesType const& rhs_indices) {
        this->patterns_table_ = patterns_table;
        this->lhs_indices_ = lhs_indices;
        this->rhs_indices_ = rhs_indices;
        for (auto& lhs_index : lhs_indices) {
            tokenizer_.TokenizeColumn(lhs_index);
        }
        for (auto& rhs_index : rhs_indices) {
            tokenizer_.TokenizeColumn(rhs_index);
        }
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