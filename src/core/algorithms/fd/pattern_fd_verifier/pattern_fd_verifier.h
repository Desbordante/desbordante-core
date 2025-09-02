#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/fd/pattern_fd_verifier/highlight.h"
#include "algorithms/fd/pattern_fd_verifier/pattern_fd_stats_calculator.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"

namespace algos::pattern_fd {

class PatternFDVerifier : public Algorithm {
private:
    config::InputTable input_table_;  // input table for verification
    config::EqNullsType is_null_equal_null_;
    config::IndicesType lhs_indices_;  // indices for LHS attributes of FD
    config::IndicesType rhs_indices_;  // indices for RHS attributes of FD
    PatternsTable patterns_table_;     // table of patterns for attributes
    int min_pattern_fd_coverage_;
    int min_pattern_inclusion_;
    double max_rhs_deviation_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::unique_ptr<PatternFDStatsCalculator> stats_calculator_;

    void ResetState() override {
        if (stats_calculator_) {
            stats_calculator_->ResetState();
        }
    }

    void RegisterOptions();
    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() override;
    unsigned long long ExecuteInternal() override;
    void VerifyPatternFD();

    void CalculateStatistics() {
        stats_calculator_->CalculateStatistics();
    }

public:
    PatternFDVerifier();

    int GetRealPatternFDCoverage() const {
        assert(stats_calculator_);
        return stats_calculator_->GetPatternFDCoverage();
    }

    int GetRealMinPatternInclusion() const {
        assert(stats_calculator_);
        return stats_calculator_->GetMinPatternInclusion();
    }

    double GetRealMaxRhsDeviation() const {
        assert(stats_calculator_);
        return stats_calculator_->GetMaxRhsDeviation();
    }

    bool PatternFDHolds() const {
        assert(stats_calculator_);
        return (stats_calculator_->GetPatternFDCoverage() >= min_pattern_fd_coverage_) &&
               (stats_calculator_->GetMinPatternInclusion() >= min_pattern_inclusion_) &&
               (stats_calculator_->GetMaxRhsDeviation() <= max_rhs_deviation_);
    }

    size_t GetNumErrorClusters() const {
        assert(stats_calculator_);
        return stats_calculator_->GetNumErrorClusters();
    }

    size_t GetNumErrorRows() const {
        assert(stats_calculator_);
        return stats_calculator_->GetNumErrorRows();
    }

    std::vector<Highlight> const& GetClusters() const {
        assert(stats_calculator_);
        return stats_calculator_->GetClusters();
    }
};

}  // namespace algos::pattern_fd