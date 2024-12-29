#include "ar_stats_calculator.h"

#include <iostream>
#include <map>
#include <queue>

namespace algos {
double ARStatsCalculator::JaccardSimilarity(std::vector<unsigned> const& transaction_indices,
                                            std::vector<unsigned> const& rule_part) {
    if (transaction_indices.empty()) return 0.0;

    std::map<unsigned, int> count_transaction;
    for (unsigned x : transaction_indices) count_transaction[x]++;
    std::map<unsigned, int> count_rule;
    for (unsigned x : rule_part) count_rule[x]++;

    long long intersection_count = 0;
    long long union_count = 0;

    for (auto const& pair_transaction : count_transaction) {
        unsigned element = pair_transaction.first;
        int count_in_transaction = pair_transaction.second;

        if (auto it_rule = count_rule.find(element); it_rule != count_rule.end()) {
            int const min_count = std::min(count_in_transaction, it_rule->second);
            intersection_count += min_count;
        }
        union_count += count_in_transaction;
    }

    for (auto const& [fst, snd] : count_rule) {
        if (!count_transaction.contains(fst)) {
            union_count += snd;
        }
    }

    if (union_count == 0) return 1.0;

    return static_cast<double>(intersection_count) / union_count;
}

size_t ARStatsCalculator::CalculateClusterPriority(std::pair<double, double> const& jaccard) {
    return 3 * static_cast<int>(std::floor(jaccard.first)) +
           2 * static_cast<int>(std::floor(jaccard.second));
}

void ARStatsCalculator::CalculateJaccardCoefficients() {
    for (auto const& [id, itemset] : data_->GetTransactions()) {
        double jaccard_left = JaccardSimilarity(itemset.GetItemsIDs(), rule_.left);

        // testing threshold
        if (double jaccard_right = JaccardSimilarity(itemset.GetItemsIDs(), rule_.right);
            jaccard_left > jaccard_threshold_ &&
            (jaccard_right > jaccard_threshold_ || jaccard_right == 0.0)) {
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
    rule_.confidence = lhs_support != 0.0 ? support_ / lhs_support : 0.0;
}

void ARStatsCalculator::ResetState() {
    jaccard_coefficients_.clear();
    support_ = 0.0;
    rule_.confidence = 0.0;
    clusters_violating_ar_.clear();
    num_transactions_violating_ar_ = 0;
}

void ARStatsCalculator::CalculateStatistics() {
    CalculateJaccardCoefficients();
    CalculateSupport();
    CalculateConfidence();
    for (auto const& [transaction_id, coef] : jaccard_coefficients_) {
        clusters_violating_ar_[CalculateClusterPriority(coef)].push_back(transaction_id);
    }
}
}  // namespace algos