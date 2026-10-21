#include "core/algorithms/fd/pattern_fd_verifier/pattern_fd_stats_calculator.h"

#include <algorithm>

namespace algos::pattern_fd {

namespace {

std::vector<std::string> ExtractValues(std::shared_ptr<PatternInfo> const& pattern,
                                       std::string const& value) {
    auto const* regex = dynamic_cast<RegexPatternInfo const*>(pattern.get());
    if (regex && regex->HasConstraints()) return regex->ExtractConstrainedParts(value);
    return {value};
}

}  // namespace

bool PatternFDStatsCalculator::Matches(config::IndexType column_index,
                                       std::shared_ptr<PatternInfo> const& pattern,
                                       std::string const& value) const {
    if (auto const* regex = dynamic_cast<RegexPatternInfo const*>(pattern.get())) {
        return regex->Matches(value);
    }
    if (auto const* token = dynamic_cast<TokenPatternInfo const*>(pattern.get())) {
        return tokenizer_.Matches(column_index, *token, value);
    }
    return true;
}

std::unordered_set<size_t> PatternFDStatsCalculator::FindLhsMatchesForRowPattern(
        std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns) {
    std::unordered_set<size_t> matched_rows;
    for (size_t row_index = 0; row_index < typed_relation_->GetNumRows(); ++row_index) {
        bool matches = true;
        for (auto const lhs_index : lhs_indices_) {
            auto it = patterns.find(lhs_index);
            if (it == patterns.end()) continue;
            std::string const value =
                    typed_relation_->GetColumnData(lhs_index).GetDataAsString(row_index);
            if (!Matches(lhs_index, it->second, value)) {
                matches = false;
                break;
            }
        }
        if (matches) matched_rows.insert(row_index);
    }
    return matched_rows;
}

void PatternFDStatsCalculator::FindLhsMatches() {
    lhs_matched_rows_.reserve(patterns_table_.size());
    for (auto const& patterns : patterns_table_) {
        lhs_matched_rows_.push_back(FindLhsMatchesForRowPattern(patterns));
    }
}

RowClusters PatternFDStatsCalculator::BuildLhsClusters(
        std::unordered_set<size_t> const& matched_rows,
        std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns) {
    RowClusters lhs_clusters;
    for (size_t row_index : matched_rows) {
        ClusterKey key;
        key.reserve(lhs_indices_.size());
        for (auto const lhs_index : lhs_indices_) {
            auto it = patterns.find(lhs_index);
            if (it == patterns.end()) continue;
            std::string const value =
                    typed_relation_->GetColumnData(lhs_index).GetDataAsString(row_index);
            key.push_back(ExtractValues(it->second, value));
        }
        lhs_clusters[key].push_back(row_index);
    }
    return lhs_clusters;
}

size_t PatternFDStatsCalculator::CheckClustersForDeviations(
        RowClusters const& lhs_clusters,
        std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns,
        std::unordered_set<size_t>& error_rows) {
    size_t total_violations = 0;
    static std::shared_ptr<PatternInfo> const kWildcard;
    for (auto const& [lhs_key, cluster_rows] : lhs_clusters) {
        std::map<ClusterKey, std::vector<size_t>> rhs_groups;
        std::vector<size_t> errors;
        for (size_t row_index : cluster_rows) {
            ClusterKey rhs_key;
            bool valid = true;
            for (auto const rhs_index : rhs_indices_) {
                auto const it = patterns.find(rhs_index);
                auto const& pattern = it != patterns.end() ? it->second : kWildcard;
                std::string const value =
                        typed_relation_->GetColumnData(rhs_index).GetDataAsString(row_index);
                auto const* regex = dynamic_cast<RegexPatternInfo const*>(pattern.get());
                if (regex) {
                    auto extracted = regex->MatchAndExtract(value);
                    if (!extracted) {
                        valid = false;
                        break;
                    }
                    if (regex->HasConstraints()) rhs_key.push_back(std::move(*extracted));
                } else if (!Matches(rhs_index, pattern, value)) {
                    valid = false;
                    break;
                } else if (!pattern || dynamic_cast<WildcardPatternInfo const*>(pattern.get())) {
                    rhs_key.push_back({value});
                }
            }
            if (valid) {
                rhs_groups[rhs_key].push_back(row_index);
            } else {
                errors.push_back(row_index);
            }
        }

        size_t max_count = 0;
        ClusterKey const* majority = nullptr;
        for (auto const& [key, rows] : rhs_groups) {
            if (rows.size() > max_count) {
                max_count = rows.size();
                majority = &key;
            }
        }
        for (auto const& [key, rows] : rhs_groups) {
            if (&key != majority) errors.insert(errors.end(), rows.begin(), rows.end());
        }

        if (!errors.empty()) {
            highlights_.emplace_back(model::PLI::Cluster(cluster_rows.begin(), cluster_rows.end()),
                                     errors);
            total_violations += errors.size();
            error_rows.insert(errors.begin(), errors.end());
        }
    }
    return total_violations;
}

void PatternFDStatsCalculator::CompareRhsValues() {
    std::unordered_set<size_t> covered_rows;
    std::unordered_set<size_t> error_rows;
    for (size_t i = 0; i < patterns_table_.size(); ++i) {
        auto const& matched_rows = lhs_matched_rows_[i];
        min_pattern_inclusion_ =
                std::min(min_pattern_inclusion_, static_cast<int>(matched_rows.size()));
        covered_rows.insert(matched_rows.begin(), matched_rows.end());
        if (matched_rows.empty()) continue;

        RowClusters lhs_clusters = BuildLhsClusters(matched_rows, patterns_table_[i]);
        size_t const violations =
                CheckClustersForDeviations(lhs_clusters, patterns_table_[i], error_rows);
        max_rhs_deviation_ =
                std::max(max_rhs_deviation_, static_cast<double>(violations) / matched_rows.size());
    }
    pattern_fd_coverage_ = static_cast<int>(covered_rows.size());
    num_error_rows_ = error_rows.size();
}

void PatternFDStatsCalculator::ResetState() {
    highlights_.clear();
    lhs_matched_rows_.clear();
    num_error_rows_ = 0;
    pattern_fd_coverage_ = 0;
    min_pattern_inclusion_ = INT_MAX;
    max_rhs_deviation_ = 0.0;
}

void PatternFDStatsCalculator::CalculateStatistics() {
    ResetState();
    FindLhsMatches();
    CompareRhsValues();
}

}  // namespace algos::pattern_fd
