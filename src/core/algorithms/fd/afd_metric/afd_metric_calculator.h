#pragma once

#include <cstddef>
#include <memory>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/fd/afd_metric/afd_metric.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/error/type.h"
#include "core/config/indices/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/position_list_index.h"

namespace algos::afd_metric_calculator {

class AFDMetricCalculator : public Algorithm {
private:
    config::InputTable input_table_;

    AFDMetric metric_;
    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;

    std::shared_ptr<ColumnLayoutRelationData> relation_;

    long double result_ = 0.L;

    void RegisterOptions();

    void ResetState() final {
        result_ = 0.L;
    }

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    void ExecuteInternal() override;

public:
    static long double CalculatePdepSelf(model::PLI const* x_pli);

    // Computes Pdep(X, X∪A) directly from X clusters and A's probing table.
    static long double CalculatePdepMeasure(model::PLI const* x_pli, model::PLI const* a_pli);

    static long double CalculateG2Error(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                        size_t num_rows);

    static long double CalculateG3(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                   size_t num_rows);

    static long double CalculateTau(model::PLI const* lhs_pli, model::PLI const* rhs_pli);

    static long double CalculateMuPlus(model::PLI const* lhs_pli, model::PLI const* rhs_pli);

    static long double CalculateFI(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                   size_t num_rows);

    static config::ErrorType CalculateZeroAryG1(ColumnData const* rhs,
                                                unsigned long long num_tuple_pairs);

    static config::ErrorType CalculateG1Error(model::PLI const* lhs_pli,
                                              model::PLI const* joint_pli,
                                              unsigned long long num_tuple_pairs);

    static config::ErrorType CalculateRhoMeasure(model::PLI const* x_pli, model::PLI const* xa_pli);

    long double GetResult() const {
        return result_;
    }

    AFDMetricCalculator();
};

}  // namespace algos::afd_metric_calculator
