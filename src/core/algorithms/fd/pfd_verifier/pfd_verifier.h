#pragma once

#include <assert.h>  // for assert
#include <memory>    // for unique_ptr
#include <stddef.h>  // for size_t
#include <vector>    // for vector

#include "algorithms/algorithm.h"                             // for Algorithm
#include "algorithms/fd/pfd_verifier/pfd_stats_calculator.h"  // for PFDStat...
#include "algorithms/fd/tane/enums.h"                         // for operator+
#include "config/equal_nulls/type.h"                          // for EqNulls...
#include "config/error_measure/type.h"                        // for PfdErro...
#include "config/indices/type.h"                              // for Indices...
#include "config/tabular_data/input_table_type.h"             // for InputTable
#include "table/position_list_index.h"                        // for PLI

class ColumnLayoutRelationData;

namespace algos {

class PFDVerifier : public Algorithm {
private:
    config::InputTable input_table_;

    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;
    config::EqNullsType is_null_equal_null_;
    config::PfdErrorMeasureType error_measure_ = +PfdErrorMeasure::per_tuple;

    std::shared_ptr<ColumnLayoutRelationData> relation_;
    std::unique_ptr<PFDStatsCalculator> stats_calculator_;

    void ResetState() override {
        if (stats_calculator_) {
            stats_calculator_->ResetState();
        }
    }

    void RegisterOptions();
    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() override;
    unsigned long long ExecuteInternal() override;
    void VerifyPFD() const;
    std::shared_ptr<model::PLI const> CalculatePLI(config::IndicesType const& indices) const;

public:
    size_t GetNumViolatingClusters() const {
        assert(stats_calculator_);
        return stats_calculator_->GetNumViolatingClusters();
    }

    size_t GetNumViolatingRows() const {
        assert(stats_calculator_);
        return stats_calculator_->GetNumViolatingRows();
    }

    std::vector<model::PLI::Cluster> const& GetViolatingClusters() const {
        assert(stats_calculator_);
        return stats_calculator_->GetViolatingClusters();
    }

    double GetError() const {
        assert(stats_calculator_);
        return stats_calculator_->GetError();
    }

    PFDVerifier();
};

}  // namespace algos
