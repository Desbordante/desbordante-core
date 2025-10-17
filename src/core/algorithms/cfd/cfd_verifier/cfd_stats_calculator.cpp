#include "cfd_stats_calculator.h"

#include <algorithm>
#include <functional>
#include <ranges>

#include "cfd/cfd_verifier/highlight.h"
#include "cfd/model/cfd_types.h"

namespace algos::cfd_verifier {
void CFDStatsCalculator::CreateSupportMask() {
    support_mask_.resize(relation_->GetNumRows(), true);

    for (size_t i = 0; i < relation_->GetNumRows(); i++) {
        cfd::Transaction const& row_values = relation_->GetRow(i);

        support_mask_[i] = std::ranges::all_of(rule_.first, [&row_values](int val) {
            return val <= 0 || std::ranges::find(row_values, val) != row_values.end();
        });
    }
}

void CFDStatsCalculator::MakeLhsToRowNums() {
    for (size_t row_idx = 0; row_idx < relation_->GetNumRows(); ++row_idx) {
        if (!support_mask_[row_idx]) {
            continue;
        }

        cfd::Itemset lhs_values;
        lhs_values.reserve(lhs_attrs_.size());
        auto const& row_values = relation_->GetRow(row_idx);
        for (int attr_idx : lhs_attrs_) {
            lhs_values.push_back(row_values[attr_idx]);
        }

        lhs_to_row_nums_[std::move(lhs_values)].push_back(row_idx);
    }
}

void CFDStatsCalculator::DetermineMostFrequentRHS() {
    for (auto const& [lhs_values, row_indices] : lhs_to_row_nums_) {
        auto rhs_values = row_indices | std::views::transform([this](size_t row_index) {
                              return relation_->GetRow(row_index)[rhs_attr_index_];
                          });

        std::unordered_map<cfd::Item, size_t> rhs_count;
        for (cfd::Item val : rhs_values) {
            rhs_count[val]++;
        }

        if (!rhs_count.empty()) {
            auto max_it = std::ranges::max_element(rhs_count, std::less{},
                                                   [](auto const& pair) { return pair.second; });
            most_frequent_rhs_[lhs_values] = max_it->first;
        }
    }
}

void CFDStatsCalculator::CalculateSupportAndConfidence() {
    size_t total_supported = 0;
    size_t total_violations = 0;

    for (auto const& [lhs_values, row_indices] : lhs_to_row_nums_) {
        auto it = most_frequent_rhs_.find(lhs_values);
        if (it == most_frequent_rhs_.end()) {
            continue;
        }

        cfd::Item most_frequent_rhs = it->second;
        std::vector<size_t> violating_rows;

        for (size_t row_index : row_indices) {
            auto const& row = relation_->GetRow(row_index);
            bool satisfies = (rule_.second < 0 && row[rhs_attr_index_] == most_frequent_rhs) ||
                             (rule_.second > 0 && row[rhs_attr_index_] == rule_.second);

            if (!satisfies) {
                violating_rows.push_back(row_index);
            }
        }
        total_supported += row_indices.size();
        total_violations += violating_rows.size();

        if (violating_rows.size() > 0) {
            highlights_.emplace_back(std::move(row_indices), std::move(violating_rows));
        }
    }
    num_rows_violating_cfd_ = total_violations;
    support_ = total_supported;
    confidence_ = total_supported > 0 ? static_cast<double>(total_supported - total_violations) /
                                                total_supported
                                      : 0.0;
}

void CFDStatsCalculator::ResetState() {
    support_mask_.clear();
    lhs_to_row_nums_.clear();
    most_frequent_rhs_.clear();
    support_ = 0;
    confidence_ = 0.0;
    num_rows_violating_cfd_ = 0;
    highlights_.clear();
}

void CFDStatsCalculator::CalculateStatistics() {
    ResetState();
    CreateSupportMask();
    MakeLhsToRowNums();
    DetermineMostFrequentRHS();
    CalculateSupportAndConfidence();
}

}  // namespace algos::cfd_verifier
