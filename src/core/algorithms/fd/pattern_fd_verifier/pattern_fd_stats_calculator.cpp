#include "pattern_fd_stats_calculator.h"

#include <map>
#include <vector>

namespace algos::pattern_fd {

std::vector<std::string> ExtractValues(std::shared_ptr<PatternInfo> const& pattern_info,
                                       std::string const& cell_value) {
    if (!pattern_info || pattern_info->Type() == "WildcardPatternInfo") {
        return {cell_value};
    }

    if (pattern_info->Type() == "RegexPatternInfo") {
        auto regex_info = std::dynamic_pointer_cast<RegexPatternInfo>(pattern_info);
        if (regex_info->HasConstraints()) {
            return regex_info->ExtractConstrainedParts(cell_value);
        }
    }

    return {cell_value};
}

void IntersectRowSets(std::unordered_set<config::IndexType>& main_set,
                      std::unordered_set<config::IndexType> const& other_set) {
    if (main_set.empty()) return;

    if (main_set.size() > other_set.size()) {
        std::unordered_set<config::IndexType> intersection;
        for (auto const& row : other_set) {
            if (main_set.count(row)) {
                intersection.insert(row);
            }
        }
        main_set = std::move(intersection);
    } else {
        auto it = main_set.begin();
        while (it != main_set.end()) {
            if (other_set.find(*it) == other_set.end()) {
                it = main_set.erase(it);
            } else {
                ++it;
            }
        }
    }
}

std::unordered_set<config::IndexType> PatternFDStatsCalculator::FindRegexMatches(
        config::IndexType lhs_index, std::shared_ptr<RegexPatternInfo> const& regex_pattern_info) {
    std::unordered_set<config::IndexType> search_space;
    auto const& literals = regex_pattern_info->GetLiterals();

    if (!literals.empty()) {
        bool first_literal = true;
        for (auto const& literal : literals) {
            TokenPatternInfo literal_pattern(literal.first, literal.second);
            auto const& tokens_for_column = tokens_map_->at(lhs_index);
            auto matched_it = tokens_for_column.find(literal_pattern);

            if (matched_it == tokens_for_column.end()) {
                return {};
            }

            if (first_literal) {
                search_space.insert(matched_it->second.begin(), matched_it->second.end());
                first_literal = false;
            } else {
                IntersectRowSets(search_space,
                                 {matched_it->second.begin(), matched_it->second.end()});
            }
            if (search_space.empty()) return {};
        }
    } else {
        size_t rows_num = typed_relation_->GetNumRows();
        search_space.reserve(rows_num);
        for (config::IndexType i = 0; i < rows_num; ++i) {
            search_space.insert(i);
        }
    }

    std::unordered_set<config::IndexType> regex_matches;
    regex_matches.reserve(search_space.size());

    std::regex re(regex_pattern_info->Regex());
    auto const& column = typed_relation_->GetColumnData(lhs_index);
    for (auto const& row_index : search_space) {
        if (std::regex_match(column.GetDataAsString(row_index), re)) {
            regex_matches.insert(row_index);
        }
    }
    return regex_matches;
}

std::unordered_set<config::IndexType> PatternFDStatsCalculator::FindLhsMatchesForRowPattern(
        std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns) {
    std::unordered_set<config::IndexType> matched_rows;
    bool first_lhs = true;

    for (auto const& lhs_index : lhs_indices_) {
        auto it = patterns.find(lhs_index);
        if (it == patterns.end() || !it->second || it->second->Type() == "WildcardPatternInfo") {
            continue;
        }

        std::unordered_set<config::IndexType> current_attr_matches;
        auto const& pattern_info = it->second;

        if (pattern_info->Type() == "TokenPatternInfo") {
            auto token_pattern_info = std::dynamic_pointer_cast<TokenPatternInfo>(pattern_info);
            auto const& tokens_for_column = tokens_map_->at(lhs_index);
            auto matched_rows_it = tokens_for_column.find(*token_pattern_info);
            if (matched_rows_it != tokens_for_column.end()) {
                current_attr_matches.insert(matched_rows_it->second.begin(),
                                            matched_rows_it->second.end());
            }
        } else if (pattern_info->Type() == "RegexPatternInfo") {
            current_attr_matches = FindRegexMatches(
                    lhs_index, std::dynamic_pointer_cast<RegexPatternInfo>(pattern_info));
        }

        if (first_lhs) {
            matched_rows = std::move(current_attr_matches);
            first_lhs = false;
        } else {
            IntersectRowSets(matched_rows, current_attr_matches);
        }

        if (matched_rows.empty()) break;
    }

    if (first_lhs) {
        size_t rows_num = typed_relation_->GetNumRows();
        matched_rows.reserve(rows_num);
        for (config::IndexType i = 0; i < rows_num; ++i) {
            matched_rows.insert(i);
        }
    }
    return matched_rows;
}

void PatternFDStatsCalculator::FindLhsMatches() {
    lhs_matched_rows_.reserve(patterns_table_.size());
    for (auto const& patterns : patterns_table_) {
        lhs_matched_rows_.emplace_back(FindLhsMatchesForRowPattern(patterns));
    }
}

std::vector<size_t> PatternFDStatsCalculator::FindDeviationsByPatternCheck(
        std::vector<config::IndexType> const& cluster_rows,
        std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns) {
    std::vector<size_t> error_rows;
    error_rows.reserve(cluster_rows.size());
    for (config::IndexType row_index : cluster_rows) {
        for (auto const& rhs_index : rhs_indices_) {
            auto it = patterns.find(rhs_index);
            if (it == patterns.end() || !it->second) continue;

            auto const& rhs_pattern_info = it->second;
            std::string const& cell_value =
                    typed_relation_->GetColumnData(rhs_index).GetDataAsString(row_index);

            bool current_attr_is_valid = true;
            if (rhs_pattern_info->Type() == "RegexPatternInfo") {
                auto regex_info = std::dynamic_pointer_cast<RegexPatternInfo>(rhs_pattern_info);
                std::regex re(regex_info->Regex());
                if (!std::regex_match(cell_value, re)) {
                    current_attr_is_valid = false;
                }
            } else if (rhs_pattern_info->Type() == "TokenPatternInfo") {
                auto token_info = std::dynamic_pointer_cast<TokenPatternInfo>(rhs_pattern_info);
                if (cell_value != token_info->Token()) {
                    current_attr_is_valid = false;
                }
            }
            if (!current_attr_is_valid) {
                error_rows.push_back(row_index);
                break;
            }
        }
    }
    return error_rows;
}

std::vector<size_t> PatternFDStatsCalculator::FindDeviationsByComparison(
        std::vector<config::IndexType> const& cluster_rows,
        std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns) {
    if (cluster_rows.size() < 2) return {};

    std::map<std::vector<std::string>, std::vector<config::IndexType>> rhs_value_groups;
    std::vector<std::string> rhs_key;

    for (auto const& row_index : cluster_rows) {
        rhs_key.clear();
        for (auto const& rhs_index : rhs_indices_) {
            auto it = patterns.find(rhs_index);
            if (it == patterns.end()) throw std::runtime_error("RHS pattern not found.");

            auto extracted = ExtractValues(
                    it->second,
                    typed_relation_->GetColumnData(rhs_index).GetDataAsString(row_index));
            rhs_key.insert(rhs_key.end(), std::make_move_iterator(extracted.begin()),
                           std::make_move_iterator(extracted.end()));
        }
        rhs_value_groups[rhs_key].push_back(row_index);
    }

    if (rhs_value_groups.size() <= 1) {
        return {};
    }

    std::vector<std::string> const* majority_rhs_value = nullptr;
    size_t max_count = 0;

    for (auto const& [value, rows] : rhs_value_groups) {
        if (rows.size() > max_count) {
            max_count = rows.size();
            majority_rhs_value = &value;
        }
    }

    std::vector<size_t> error_rows;
    error_rows.reserve(cluster_rows.size() - max_count);

    for (auto const& [value, rows] : rhs_value_groups) {
        if (&value != majority_rhs_value) {
            error_rows.insert(error_rows.end(), rows.begin(), rows.end());
        }
    }

    return error_rows;
}

size_t PatternFDStatsCalculator::CheckClustersForDeviations(
        std::map<std::vector<std::string>, std::vector<config::IndexType>> const& lhs_clusters,
        std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns) {
    size_t total_violations = 0;
    for (auto const& [lhs_key, cluster_rows] : lhs_clusters) {
        bool is_rhs_pattern_check = false;
        bool is_rhs_comparison_check = false;
        for (auto const& rhs_index : rhs_indices_) {
            auto it = patterns.find(rhs_index);
            if (it == patterns.end() || !it->second) continue;
            auto const& type = it->second->Type();
            if (type == "WildcardPatternInfo") {
                is_rhs_comparison_check = true;
            } else if (type == "RegexPatternInfo") {
                std::dynamic_pointer_cast<RegexPatternInfo>(it->second)->HasConstraints()
                        ? is_rhs_comparison_check = true
                        : is_rhs_pattern_check = true;
            } else if (type == "TokenPatternInfo") {
                is_rhs_pattern_check = true;
            }
        }

        std::vector<size_t> error_rows_in_cluster;
        if (is_rhs_pattern_check) {
            error_rows_in_cluster = FindDeviationsByPatternCheck(cluster_rows, patterns);
        } else if (is_rhs_comparison_check) {
            error_rows_in_cluster = FindDeviationsByComparison(cluster_rows, patterns);
        }

        if (!error_rows_in_cluster.empty()) {
            highlights_.emplace_back(
                    model::PLI::Cluster({cluster_rows.begin(), cluster_rows.end()}),
                    error_rows_in_cluster);
            total_violations += error_rows_in_cluster.size();
        }
    }
    return total_violations;
}

std::map<std::vector<std::string>, std::vector<config::IndexType>>
PatternFDStatsCalculator::BuildLhsClusters(
        std::unordered_set<config::IndexType> const& matched_rows,
        std::unordered_map<config::IndexType, std::shared_ptr<PatternInfo>> const& patterns) {
    std::map<std::vector<std::string>, std::vector<config::IndexType>> lhs_clusters;
    for (config::IndexType row_index : matched_rows) {
        std::vector<std::string> lhs_key;
        lhs_key.reserve(lhs_indices_.size());
        bool key_extraction_failed = false;
        for (auto const& lhs_index : lhs_indices_) {
            auto it = patterns.find(lhs_index);
            if (it == patterns.end()) continue;
            auto extracted_parts = ExtractValues(
                    it->second,
                    typed_relation_->GetColumnData(lhs_index).GetDataAsString(row_index));
            if (extracted_parts.empty() && it->second && it->second->Type() == "RegexPatternInfo" &&
                std::dynamic_pointer_cast<RegexPatternInfo>(it->second)->HasConstraints()) {
                key_extraction_failed = true;
                break;
            }
            lhs_key.insert(lhs_key.end(), std::make_move_iterator(extracted_parts.begin()),
                           std::make_move_iterator(extracted_parts.end()));
        }
        if (!key_extraction_failed) {
            lhs_clusters[lhs_key].push_back(row_index);
        }
    }
    return lhs_clusters;
}

void PatternFDStatsCalculator::CompareRhsValues() {
    for (size_t i = 0; i < patterns_table_.size(); ++i) {
        auto const& matched_rows = lhs_matched_rows_[i];
        if (matched_rows.empty()) {
            min_pattern_inclusion_ = std::min(min_pattern_inclusion_, 0);
            continue;
        }
        min_pattern_inclusion_ =
                std::min(min_pattern_inclusion_, static_cast<int>(matched_rows.size()));
        pattern_fd_coverage_ += static_cast<int>(matched_rows.size());

        auto const& patterns = patterns_table_[i];
        auto lhs_clusters = BuildLhsClusters(matched_rows, patterns);

        size_t total_violations_for_pattern = CheckClustersForDeviations(lhs_clusters, patterns);
        num_error_rows_ += total_violations_for_pattern;

        if (!matched_rows.empty() && total_violations_for_pattern > 0) {
            double current_deviation =
                    static_cast<double>(total_violations_for_pattern) / matched_rows.size();
            max_rhs_deviation_ = std::max(max_rhs_deviation_, current_deviation);
        }
    }
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