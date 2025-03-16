#include "ar_stats_calculator.h"

#include <queue>
#include <set>

#include <easylogging++.h>

namespace algos::ar_verifier {
double ARStatsCalculator::CalculateTransactionCoverage(
        std::vector<unsigned> const& transaction_indices, std::vector<unsigned> const& rule_part) {
    if (transaction_indices.empty()) {
        return 0.0;
    }

    if (rule_part.empty()) {
        return 1.0;
    }

    std::set transaction_set(transaction_indices.begin(), transaction_indices.end());
    std::set rule_set(rule_part.begin(), rule_part.end());

    std::vector<unsigned> intersection;
    std::ranges::set_intersection(rule_set, transaction_set, std::back_inserter(intersection));

    return intersection.size() / static_cast<double>(rule_set.size());
}

ClusterPriority ARStatsCalculator::CalculateClusterPriority(
        std::pair<double, double> const& coverage) {
    bool const is_left_full = coverage.first == 1.0;
    bool const is_right_full = coverage.second == 1.0;
    bool const is_right_present = coverage.second > 0.0;

    if (is_left_full) {
        if (is_right_present) {
            if (is_right_full) return ClusterPriority::full_left_full_right;
            return ClusterPriority::full_left_partial_right;
        }
        return ClusterPriority::full_left_no_right;
    }
    if (is_right_present) {
        if (is_right_full) return ClusterPriority::partial_left_full_right;
        return ClusterPriority::partial_left_partial_right;
    }
    return ClusterPriority::partial_left_no_right;
}

void ARStatsCalculator::CalculateRuleCoverageCoefficients() {
    for (auto const& [id, itemset] : data_->GetTransactions()) {
        double coverage_left = CalculateTransactionCoverage(itemset.GetItemsIDs(), rule_.left);
        double coverage_right = CalculateTransactionCoverage(itemset.GetItemsIDs(), rule_.right);
        if (coverage_left != 0.0) {
            rule_coverage_coefficients_[id] = std::make_pair(coverage_left, coverage_right);
        }
    }
}

void ARStatsCalculator::CalculateSupport() {
    support_ =
            std::ranges::count_if(rule_coverage_coefficients_,
                                  [&](auto const& pair) {
                                      return pair.second.first == 1.0 && pair.second.second == 1.0;
                                  }) /
            static_cast<double>(data_->GetTransactions().size());
}

void ARStatsCalculator::CalculateConfidence() {
    double const lhs_support =
            std::ranges::count_if(rule_coverage_coefficients_,
                                  [&](auto const& pair) { return pair.second.first == 1.0; }) /
            static_cast<double>(data_->GetTransactions().size());
    confidence_ = lhs_support != 0.0 ? support_ / lhs_support : 0.0;
}

void ARStatsCalculator::ResetState() {
    rule_coverage_coefficients_.clear();
    support_ = 0.0;
    confidence_ = 0.0;
    clusters_violating_ar_.clear();
    num_transactions_violating_ar_ = 0;
}

void ARStatsCalculator::CalculateStatistics() {
    CalculateRuleCoverageCoefficients();
    CalculateSupport();
    CalculateConfidence();
    for (auto const& [transaction_id, coefficients] : rule_coverage_coefficients_) {
        ClusterPriority priority = CalculateClusterPriority(coefficients);
        if (priority != static_cast<ClusterPriority>(ClusterPriority::full_left_full_right)) {
            clusters_violating_ar_[priority._to_string()].push_back(transaction_id);
        }
    }

    for (auto const& [priority, cluster] : clusters_violating_ar_) {
        num_transactions_violating_ar_ += cluster.size();
    }
}
}  // namespace algos::ar_verifier