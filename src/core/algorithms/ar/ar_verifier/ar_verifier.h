#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/ar/ar.h"
#include "core/algorithms/ar/ar_verifier/ar_stats_calculator.h"
#include "core/config/ar_minimum_conf/type.h"
#include "core/config/ar_minimum_support/type.h"

namespace algos::ar_verifier {

/* Algorithm used to verify that AR holds on dataset and retrieving useful information in
 * case it is not. */
class ARVerifier final : public Algorithm {
private:
    /* input options */
    ::model::TransactionalData::Params transactional_data_params_;

    std::shared_ptr<::model::TransactionalData> transactional_data_;
    std::vector<std::string> string_rule_left_;
    std::vector<std::string> string_rule_right_;
    ::model::ArIDs ar_ids_;

    ARStatsCalculator stats_calculator_;

    config::ArMinimumSupportType minsup_ = 0.0;
    config::ArMinimumConfidenceType minconf_ = 0.0;

    void VerifyAR();
    void RegisterOptions();
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

    void ResetState() override {
        stats_calculator_.ResetState();
        ar_ids_ = ::model::ArIDs();
    };

    void CalculateStatistics() {
        stats_calculator_.CalculateStatistics();
    }

public:
    /* Returns true if AR holds and false otherwise. */
    bool ARHolds() const {
        return (stats_calculator_.GetSupport() >= minsup_) &&
               (stats_calculator_.GetConfidence() >= minconf_);
    }

    /* Returns the number of clusters where the AR is violated. */
    size_t GetNumClustersViolatingAR() const {
        return stats_calculator_.GetNumClustersViolatingAR();
    }

    /* Returns the total number of table rows that satisfy the AR. */
    size_t GetNumTransactionsSatisfyingAR() const {
        return stats_calculator_.GetNumTransactionsSatisfyingAR();
    }

    /* Returns the total number of transactions that violate the AR. */
    size_t GetNumTransactionsViolatingAR() const {
        return stats_calculator_.GetNumTransactionsViolatingAR();
    }

    /* Returns clusters where the AR is violated. */
    std::unordered_map<std::string, ::model::PLI::Cluster> const& GetClustersViolatingAR() const {
        return stats_calculator_.GetClustersViolatingAR();
    }

    /* Returns an actual value of support. */
    double GetRealSupport() const {
        return stats_calculator_.GetSupport();
    }

    /* Returns an actual value of confidence. */
    double GetRealConfidence() const {
        return stats_calculator_.GetConfidence();
    }

    ARVerifier();
};
}  // namespace algos::ar_verifier
