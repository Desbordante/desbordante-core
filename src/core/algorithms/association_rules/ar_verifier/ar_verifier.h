#pragma once

#include "algorithms/algorithm.h"
#include "algorithms/association_rules/ar.h"
#include "algorithms/association_rules/ar_algorithm_enums.h"
#include "algorithms/association_rules/ar_verifier/ar_stats_calculator.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_relation_data.h"

namespace algos {
class ARVerifier : public Algorithm {
private:
    /* input options */
    config::InputTable input_table_;
    InputFormat input_format_ = InputFormat::singular;
    unsigned int tid_column_index_;
    unsigned int item_column_index_;
    bool first_column_tid_;

    std::shared_ptr<model::TransactionalData> transactional_data_;
    std::list<std::string> string_rule_left_;
    std::list<std::string> string_rule_right_;
    model::ArIDs ar_ids_;

    ARStatsCalculator stats_calculator_;
    /* results of work */
    size_t num_transactions_violating_ar_ = 0;
    std::vector<model::PLI::Cluster> clusters_violating_ar_;

    double minsup_;
    double minconf_;

    void VerifyAR();
    void RegisterOptions();
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

    void ResetState() override {
        stats_calculator_.ResetState();
    };

    void CalculateStatistics() {
        stats_calculator_.CalculateStatistics();
    }

public:
    /* Returns true if AR holds and false otherwise */
    bool ARHolds() const {
        return stats_calculator_.GetConfidence() <= minsup_ &&
               stats_calculator_.GetSupport() >= minconf_;
    }

    /* Returns the number of clusters where the AR is violated */
    size_t GetNumClustersViolatingAR() const {
        return stats_calculator_.GetNumClustersViolatingAR();
    }

    /* Returns the total number of transactions that violate the AR */
    size_t GetNumTransactionsViolatingAR() const {
        return stats_calculator_.GetNumTransactionsViolatingAR();
    }

    /* Returns clusters where the AR is violated, that is, sets of rows where each set consists of
     * rows equal to each other in the specified columns */
    std::unordered_map<size_t, model::PLI::Cluster> const& GetClustersViolatingAR() const {
        return stats_calculator_.GetClustersViolatingAR();
    }

    double GetRealSupport() const {
        return stats_calculator_.GetSupport();
    }

    double GetRealConfidence() const {
        return stats_calculator_.GetConfidence();
    }

    ARVerifier();
};
}  // namespace algos