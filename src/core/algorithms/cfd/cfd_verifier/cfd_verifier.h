#pragma once

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

    int GetSupport() const {
        return stats_calculator_.GetSupport();
    };

    double GetConfidence() const {
        return stats_calculator_.GetConfidence();
    };

    bool CFDHolds(double threshold = 0.9) const {
        return GetConfidence() >= threshold;
    };

    std::vector<size_t> GetRowsViolatingCFD() const {
        return stats_calculator_.GetRowsViolatingCFD();
    }
};

}  // namespace algos::cfd_verifier
