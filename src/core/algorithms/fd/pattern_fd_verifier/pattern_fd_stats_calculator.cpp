#include "pattern_fd_stats_calculator.h"

namespace algos::pattern_fd {

void PatternFDStatsCalculator::ResetState() {
    clusters_.clear();
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

void PatternFDStatsCalculator::FindLhsMatches() {
    for (auto const &patterns : patterns_table_) {
        std::unordered_set<config::IndexType> lhs_matched_rows;
        for (auto const &lhs : lhs_indices_) {
            auto it = patterns.find(lhs);
            if (it == patterns.end()) {
                throw std::runtime_error("Pattern not found for LHS index: " + std::to_string(lhs));
            }
            auto const &pattern_info = it->second;
            if (!pattern_info) {
                continue;
            }

            if (pattern_info->Type() == "TokenPatternInfo") {
                auto it = tokens_map_->find(lhs);
                if (it == tokens_map_->end()) {
                    throw std::runtime_error(
                            "This LHS index not tokenized, may be wrong attribute type or constant "
                            "value: " +
                            std::to_string(lhs));
                }
                auto &tokens = it->second;

                auto matched_rows =
                        tokens.find(*std::dynamic_pointer_cast<TokenPatternInfo>(pattern_info));
                if (matched_rows == tokens.end()) {
                    lhs_matched_rows.clear();
                    break;
                }
                if (lhs_matched_rows.empty()) {
                    lhs_matched_rows.insert(matched_rows->second.begin(),
                                            matched_rows->second.end());
                } else {
                    std::unordered_set<config::IndexType> intersection;
                    for (auto const &row : matched_rows->second) {
                        if (lhs_matched_rows.find(row) != lhs_matched_rows.end()) {
                            intersection.insert(row);
                        }
                    }
                    lhs_matched_rows = std::move(intersection);
                }
            } else {  // regex pattern
                auto regex_pattern_info = std::dynamic_pointer_cast<RegexPatternInfo>(pattern_info);

                std::string const &regex_pattern = regex_pattern_info->Regex();
                std::regex re(regex_pattern);

                auto const &column = typed_relation_->GetColumnData(lhs);
                size_t rows_num = typed_relation_->GetNumRows();

                std::unordered_set<config::IndexType> matched_rows;
                for (config::IndexType row_index = 0; row_index < rows_num; ++row_index) {
                    std::string value = column.GetDataAsString(row_index);
                    if (std::regex_match(value, re)) {
                        matched_rows.insert(row_index);
                    }
                }

                if (matched_rows.empty()) {
                    lhs_matched_rows.clear();
                    break;
                }

                if (lhs_matched_rows.empty()) {
                    lhs_matched_rows = std::move(matched_rows);
                } else {
                    std::unordered_set<config::IndexType> intersection;
                    for (auto const &row : matched_rows) {
                        if (lhs_matched_rows.find(row) != lhs_matched_rows.end()) {
                            intersection.insert(row);
                        }
                    }
                    lhs_matched_rows = std::move(intersection);
                }
            }
        }
        lhs_matched_rows_.emplace_back(lhs_matched_rows);
    }
}

void PatternFDStatsCalculator::CompareRhsValues() {
    for (size_t pattern_number = 0; pattern_number < patterns_table_.size(); ++pattern_number) {
        auto const &patterns = patterns_table_[pattern_number];
        std::unordered_set<config::IndexType> rhs_matched_rows;
        auto it = patterns.find(rhs_index_);
        if (it == patterns.end()) {
            throw std::runtime_error("Pattern not found for RHS index: " +
                                     std::to_string(rhs_index_));
        }
        auto const &rhs_pattern_info = it->second;
        if (!rhs_pattern_info) {
            continue;
        }

        if (rhs_pattern_info->Type() == "TokenPatternInfo") {
            auto token_pattern_info = std::dynamic_pointer_cast<TokenPatternInfo>(rhs_pattern_info);
            auto it = tokens_map_->find(rhs_index_);
            if (it == tokens_map_->end()) {
                throw std::runtime_error(
                        "RHS index not tokenized, may be wrong attribute type or constant value: " +
                        std::to_string(rhs_index_));
            }
            auto &tokens = it->second;

            auto token_it =
                    tokens.find(*std::dynamic_pointer_cast<TokenPatternInfo>(token_pattern_info));
            if (token_it != tokens.end()) {
                auto const &matched_rows = token_it->second;
                rhs_matched_rows.insert(matched_rows.begin(), matched_rows.end());
            }
        } else {
            auto regex_pattern_info = std::dynamic_pointer_cast<RegexPatternInfo>(rhs_pattern_info);

            std::string const &regex_pattern = regex_pattern_info->Regex();
            std::regex re(regex_pattern);

            auto const &column = typed_relation_->GetColumnData(rhs_index_);
            size_t rows_num = typed_relation_->GetNumRows();
            for (config::IndexType row_index = 0; row_index < rows_num; ++row_index) {
                std::string value = column.GetDataAsString(row_index);
                if (std::regex_match(value, re)) {
                    rhs_matched_rows.insert(row_index);
                }
            }
        }
        std::unordered_set<config::IndexType> &lhs_matched_rows = lhs_matched_rows_[pattern_number];
        std::unordered_set<config::IndexType> intersection;
        for (auto const &row : lhs_matched_rows) {
            if (rhs_matched_rows.find(row) != rhs_matched_rows.end()) {
                intersection.insert(row);
            }
        }

        std::unordered_set<config::IndexType> unmatched_rows;
        for (auto const &row : lhs_matched_rows) {
            if (intersection.find(row) == intersection.end()) {
                unmatched_rows.insert(row);
            }
        }

        min_pattern_inclusion_ =
                std::min(min_pattern_inclusion_, static_cast<int>(lhs_matched_rows.size()));
        if (!lhs_matched_rows.empty()) {
            max_rhs_deviation_ =
                    std::max(max_rhs_deviation_,
                             static_cast<double>(unmatched_rows.size()) / lhs_matched_rows.size());
        }
        pattern_fd_coverage_ += static_cast<int>(lhs_matched_rows.size());

        if (unmatched_rows.empty()) {
            continue;
        }

        highlights_.emplace_back(model::PLI::Cluster(std::vector<int>(lhs_matched_rows.begin(),
                                                                      lhs_matched_rows.end())),
                                 std::vector<size_t>(unmatched_rows.begin(), unmatched_rows.end()));
        clusters_.emplace_back(model::PLI::Cluster(
                std::vector<int>(unmatched_rows.begin(), unmatched_rows.end())));
        num_error_rows_ += unmatched_rows.size();
    }
}

}  // namespace algos::pattern_fd