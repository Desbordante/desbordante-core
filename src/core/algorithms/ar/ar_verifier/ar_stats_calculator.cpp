#include "core/algorithms/ar/ar_verifier/ar_stats_calculator.h"

#include <ranges>

#include "core/util/enum_to_str.h"

namespace algos::ar_verifier {
ClusterPriority ARStatsCalculator::CalculateClusterPriority(model::RuleCoverage const& coverage) {
    if (coverage.IsLeftFull()) {
        if (coverage.IsRightPresented()) {
            if (coverage.IsRightFull()) return ClusterPriority::kFullLeftFullRight;
            return ClusterPriority::kFullLeftPartialRight;
        }
        return ClusterPriority::kFullLeftNoRight;
    }
    if (coverage.IsRightPresented()) {
        if (coverage.IsRightFull()) return ClusterPriority::kPartialLeftFullRight;
        return ClusterPriority::kPartialLeftPartialRight;
    }
    return ClusterPriority::kPartialLeftNoRight;
}

void ARStatsCalculator::CalculateRuleCoverageCoefficients() {
    for (auto const& [id, itemset] : data_->GetTransactions()) {
        if (auto const coverage =
                    model::RuleCoverage(itemset.GetItemsIDs(), rule_.left, rule_.right);
            coverage.IsLeftPresented()) {
            rule_coverage_coefficients_[id] = coverage;
        }
    }
}

void ARStatsCalculator::CalculateSupport() {
    support_ = static_cast<double>(std::ranges::count_if(
                       std::views::values(rule_coverage_coefficients_),
                       [&](auto const& coverage) { return coverage.IsRuleFull(); })) /
               static_cast<double>(data_->GetTransactions().size());
}

void ARStatsCalculator::CalculateConfidence() {
    double const lhs_support =
            static_cast<double>(std::ranges::count_if(
                    std::views::values(rule_coverage_coefficients_),
                    [&](auto const& coverage) { return coverage.IsLeftFull(); })) /
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
        if (ClusterPriority priority = CalculateClusterPriority(coefficients);
            priority != ClusterPriority::kFullLeftFullRight) {
            clusters_violating_ar_[util::EnumToStr(priority)].push_back(transaction_id);
        }
    }

    for (auto const& cluster : clusters_violating_ar_ | std::views::values) {
        num_transactions_violating_ar_ += cluster.size();
    }
}
}  // namespace algos::ar_verifier
