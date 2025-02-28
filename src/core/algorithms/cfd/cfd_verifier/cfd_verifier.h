#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/algorithm.h"
#include "cfd_stats_calculator.h"
#include "config/tabular_data/input_table_type.h"

namespace algos::cfd_verifier {

class CFDVerifier : public Algorithm {
private:
    config::InputTable input_table_;

    std::vector<std::pair<std::string, std::string>> string_rule_left_;
    std::pair<std::string, std::string> string_rule_right_;
    cfd::ItemsetCFD cfd_;

    int minsup_ = 0;
    double minconf_ = 0.0;

    std::shared_ptr<cfd::CFDRelationData> relation_;
    CFDStatsCalculator stats_calculator_;

    void VerefyCFD();

    void RegisterOptions();

    void ResetState() override {
        stats_calculator_.ResetState();
    };

    void CalculateStatistics() {
        stats_calculator_.CalculateStatistics();
    }

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    CFDVerifier();

    int GetRealSupport() const {
        return stats_calculator_.GetSupport();
    };

    double GetRealConfidence() const {
        return stats_calculator_.GetConfidence();
    };

    bool CFDHolds() const {
        return (stats_calculator_.GetSupport() >= minsup_) &&
               (stats_calculator_.GetConfidence() >= minconf_);
    };

    std::vector<size_t> GetRowsSatisfyingCFD() const {
        return stats_calculator_.GetRowsSatisfyingCFD();
    }

    std::vector<size_t> GetRowsViolatingCFD() const {
        return stats_calculator_.GetRowsViolatingCFD();
    }
};

}  // namespace algos::cfd_verifier
