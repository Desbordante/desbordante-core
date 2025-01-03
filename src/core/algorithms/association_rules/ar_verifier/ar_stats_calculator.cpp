#include "ar_stats_calculator.h"

#include <iostream>
#include <map>
#include <queue>
#include <set>

namespace algos {
double ARStatsCalculator::JaccardSimilarity(std::vector<unsigned> const& transaction_indices,
                                            std::vector<unsigned> const& rule_part) {
    if (transaction_indices.empty()) {
        return 0.0;
    }

    std::set transaction_set(transaction_indices.begin(), transaction_indices.end());
    std::set rule_set(rule_part.begin(), rule_part.end());

    std::vector<unsigned> intersection;
    std::ranges::set_intersection(transaction_set, rule_set, std::back_inserter(intersection));
    unsigned const intersection_count = intersection.size();

    std::vector<unsigned> union_set;
    std::ranges::set_union(transaction_set, rule_set, std::back_inserter(union_set));
    unsigned const union_count = union_set.size();

    if (union_count == 0) {
        return 1.0;
    }
    return static_cast<double>(intersection_count) / union_count;
}

size_t ARStatsCalculator::CalculateClusterPriority(std::pair<double, double> const& jaccard) {
    return static_cast<int>(std::floor(3 * jaccard.first)) +
           static_cast<int>(std::floor(2 * jaccard.second));
}

void ARStatsCalculator::CalculateJaccardCoefficients() {
    for (auto const& [id, itemset] : data_->GetTransactions()) {
        double jaccard_left = JaccardSimilarity(itemset.GetItemsIDs(), rule_.left);
        double jaccard_right = JaccardSimilarity(itemset.GetItemsIDs(), rule_.right);
        if (jaccard_left != 0.0) {
            jaccard_coefficients_[id] = std::make_pair(jaccard_left, jaccard_right);
        }
    }
}

void ARStatsCalculator::CalculateSupport() {
    support_ =
            std::ranges::count_if(jaccard_coefficients_,
                                  [&](auto const& pair) {
                                      return pair.second.first == 1.0 && pair.second.second == 1.0;
                                  }) /
            data_->GetTransactions().size();
}

void ARStatsCalculator::CalculateConfidence() {
    double lhs_support =
            std::ranges::count_if(jaccard_coefficients_,
                                  [&](auto const& pair) { return pair.second.first == 1.0; }) /
            data_->GetTransactions().size();
    confidence_ = lhs_support != 0.0 ? support_ / lhs_support : 0.0;
}

void ARStatsCalculator::ResetState() {
    jaccard_coefficients_.clear();
    support_ = 0.0;
    confidence_ = 0.0;
    clusters_violating_ar_.clear();
    num_transactions_violating_ar_ = 0;
}

void ARStatsCalculator::CalculateStatistics() {
    CalculateJaccardCoefficients();
    CalculateSupport();
    CalculateConfidence();
    for (auto const& [transaction_id, coefficients] : jaccard_coefficients_) {
        clusters_violating_ar_[CalculateClusterPriority(coefficients)].push_back(transaction_id);
    }

    for (auto const& [order, cluster] : clusters_violating_ar_) {
        num_transactions_violating_ar_ += cluster.size();
    }
}
}  // namespace algos