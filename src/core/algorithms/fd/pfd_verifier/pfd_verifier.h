#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/fd/pfd_verifier/pfd_stats_calculator.h"
#include "algorithms/fd/tane/enums.h"
#include "config/equal_nulls/type.h"
#include "config/error/type.h"
#include "config/error_measure/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "table/position_list_index.h"

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
