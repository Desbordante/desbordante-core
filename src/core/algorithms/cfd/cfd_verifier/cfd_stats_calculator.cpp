#include "cfd_stats_calculator.h"

#include <iostream>

#include "cfd/util/cfd_output_util.h"

namespace algos::cfd_verifier {
void CFDStatsCalculator::CreateSupportMask() {
    support_mask_.resize(relation_->GetNumRows(), true);

    for (size_t i = 0; i < relation_->GetNumRows(); i++) {
        bool supports_cfd = true;

        std::vector<int> row_values = relation_->GetRow(i);

        for (auto expected_value : rule_.first) {
            if (std::find(row_values.begin(), row_values.end(), expected_value) ==
                        row_values.end() &&
                expected_value > 0) {
                supports_cfd = false;
                break;
            }
        }

        support_mask_[i] = supports_cfd;
    }
}

void CFDStatsCalculator::MakeLhsToRowNums() {
    for (size_t row_idx = 0; row_idx < relation_->GetNumRows(); ++row_idx) {
        if (!support_mask_[row_idx]) {
            continue;
        }

        cfd::Itemset lhs_values;
        auto const& row_values = relation_->GetRow(row_idx);
        for (int attr_idx : lhs_attrs_) {
            lhs_values.push_back(row_values[attr_idx]);
        }

        lhs_to_row_nums_[lhs_values].push_back(row_idx);
    }
}

void CFDStatsCalculator::DetermineMostFrequentRHS() {
    for (auto const& [lhs_values, row_indices] : lhs_to_row_nums_) {
        std::map<cfd::Item, size_t> rhs_count;

        for (size_t row_index : row_indices) {
            auto row = relation_->GetRow(row_index);
            cfd::Item rhs_value = row[rhs_attr_index_];
            rhs_count[rhs_value]++;
        }

        if (!rhs_count.empty()) {
            most_frequent_rhs_[lhs_values] = std::max_element(rhs_count.begin(), rhs_count.end(),
                                                              [](auto const& a, auto const& b) {
                                                                  return a.second < b.second;
                                                              })
                                                     ->first;
        }
    }
}

void CFDStatsCalculator::CalculateSupportAndConfidence() {
    size_t total_supported = 0;
    size_t valid_tuples = 0;

    for (auto const& [lhs_values, row_indices] : lhs_to_row_nums_) {
        auto it = most_frequent_rhs_.find(lhs_values);
        if (it == most_frequent_rhs_.end()) {
            continue;
        }

        cfd::Item most_frequent_rhs = it->second;

        for (size_t row_index : row_indices) {
            auto const& row = relation_->GetRow(row_index);
            bool satisfies = (rule_.second < 0 && row[rhs_attr_index_] == most_frequent_rhs) ||
                             (rule_.second > 0 && row[rhs_attr_index_] == rule_.second);

            if (satisfies) {
                satisfying_rows_.push_back(row_index);
                valid_tuples++;
            } else {
                violating_rows_.push_back(row_index);
            }
        }
        total_supported += row_indices.size();
    }

    std::sort(satisfying_rows_.begin(), satisfying_rows_.end());
    std::sort(violating_rows_.begin(), violating_rows_.end());

    support_ = total_supported;
    confidence_ = total_supported > 0 ? static_cast<double>(valid_tuples) / total_supported : 0.0;
}

void CFDStatsCalculator::ResetState() {
    support_mask_.clear();
    lhs_to_row_nums_.clear();
    most_frequent_rhs_.clear();
    support_ = 0;
    confidence_ = 0.0;
    violating_rows_.clear();
}

void CFDStatsCalculator::CalculateStatistics() {
    ResetState();
    CreateSupportMask();
    MakeLhsToRowNums();
    DetermineMostFrequentRHS();
    CalculateSupportAndConfidence();
}

}  // namespace algos::cfd_verifier
