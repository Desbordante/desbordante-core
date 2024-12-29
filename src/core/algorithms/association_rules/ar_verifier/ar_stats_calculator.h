#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "algorithms/association_rules/ar.h"
#include "model/table/column_layout_relation_data.h"

namespace algos {
class ARStatsCalculator {
private:
    std::shared_ptr<model::TransactionalData> data_;

    model::ArIDs rule_;
    double support_ = 0.0;
    size_t num_transactions_violating_ar_ = 0;
    std::unordered_map<size_t, std::pair<double, double>> jaccard_coefficients_;
    double jaccard_threshold_ = (rule_.left.size() - 2) / rule_.left.size();  // testing
    std::unordered_map<size_t, model::PLI::Cluster> clusters_violating_ar_;

    static double JaccardSimilarity(std::vector<unsigned> const& transaction_indices,
                                    std::vector<unsigned> const& rule_part);
    static size_t CalculateClusterPriority(std::pair<double, double> const& jaccard);
    void CalculateJaccardCoefficients();
    void CalculateSupport();
    void CalculateConfidence();

public:
    ARStatsCalculator(std::shared_ptr<model::TransactionalData>&& data, model::ArIDs&& rule)
        : data_(std::move(data)), rule_(std::move(rule)) {};

    ARStatsCalculator() = default;

    void CalculateStatistics();

    void ResetState();

    size_t GetNumClustersViolatingAR() const {
        return clusters_violating_ar_.size();
    }

    /* Returns the total number of table rows that violate the AR */
    size_t GetNumTransactionsViolatingAR() const {
        return num_transactions_violating_ar_;
    }

    /* Returns clusters where the AR is violated */
    std::unordered_map<size_t, model::PLI::Cluster> const& GetClustersViolatingAR() const {
        return clusters_violating_ar_;
    }

    double GetSupport() const {
        return support_;
    }

    double GetConfidence() const {
        return rule_.confidence;
    }
};

}  // namespace algos