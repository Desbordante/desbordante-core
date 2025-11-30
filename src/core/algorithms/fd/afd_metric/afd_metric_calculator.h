#pragma once

#include <cstddef>
#include <memory>

#include "algorithms/algorithm.h"
#include "algorithms/fd/afd_metric/afd_metric.h"
#include "algorithms/fd/tane/afd_measures.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/position_list_index.h"
#include "model/table/position_list_index_with_singletons.h"

namespace algos::afd_metric_calculator {

class AFDMetricCalculator : public Algorithm {
private:
    config::InputTable input_table_;

    AFDMetric metric_ = AFDMetric::_values()[0];
    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;
    config::EqNullsType is_null_equal_null_;

    std::shared_ptr<ColumnLayoutRelationData> relation_;

    long double result_ = 0.L;

    void CalculateMetric();

    void RegisterOptions();

    void ResetState() final {
        result_ = 0.L;
    }

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    static std::pair<long double, long double> CalculateP1P2(
            size_t num_rows, std::deque<model::PositionListIndex::Cluster>&& lhs_clusters,
            std::deque<model::PositionListIndex::Cluster>&& rhs_clusters);

    static long double CalculatePdepSelf(model::PLIWithSingletons const* x_pli);

    static long double CalculatePdepMeasure(model::PLIWithSingletons const* x_pli,
                                            model::PLIWithSingletons const* xa_pli);

    static long double CalculateG2(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                   size_t num_rows);

    static long double CalculateTau(model::PLIWS const* lhs_pli, model::PLIWS const* rhs_pli,
                                    model::PLIWS const* joint_pli);

    static long double CalculateMuPlus(model::PLIWS const* lhs_pli, model::PLIWS const* rhs_pli,
                                       model::PLIWS const* joint_pli);

    static long double CalculateFI(model::PLIWS const* lhs_pli, model::PLIWS const* rhs_pli,
                                   size_t num_rows);

    long double GetResult() const {
        return result_;
    }

    AFDMetricCalculator();
};

}  // namespace algos::afd_metric_calculator
