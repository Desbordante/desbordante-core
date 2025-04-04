#include "cfd_stats_calculator.h"

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
    size_t total_violations = 0;

    for (auto const& [lhs_values, row_indices] : lhs_to_row_nums_) {
        auto it = most_frequent_rhs_.find(lhs_values);
        if (it == most_frequent_rhs_.end()) {
            continue;
        }

        cfd::Item most_frequent_rhs = it->second;
        size_t num_violations = 0;
        std::vector<size_t> violating_rows;

        for (size_t row_index : row_indices) {
            auto const& row = relation_->GetRow(row_index);
            bool satisfies = (rule_.second < 0 && row[rhs_attr_index_] == most_frequent_rhs) ||
                             (rule_.second > 0 && row[rhs_attr_index_] == rule_.second);

            if (!satisfies) {
                num_violations++;
                violating_rows.push_back(row_index);
            }
        }
        total_supported += row_indices.size();
        total_violations += num_violations;

        if (num_violations > 0) {
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
