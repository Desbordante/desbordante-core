#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/fastdd/util/distance_calculator.h"
#include "core/model/table/column_index.h"
#include "core/model/table/column_layout_typed_relation_data.h"

namespace algos::dd {

class ThresholdCalculator {
private:
    std::shared_ptr<DistanceCalculator> distance_calculator_;

    std::vector<std::vector<model::DFConstraint>> thresholds_;
    unsigned num_rows_;

    void GetThresholds(model::TypedColumnData const& dif_column, model::ColumnIndex column_index);
    std::vector<std::size_t> SampleRows(std::size_t row_limit) const;
    std::pair<std::vector<double>, std::vector<double>> SampleThresholds(
            model::ColumnIndex const column_index, std::vector<std::size_t> const& row_nums,
            std::size_t row_limit, std::size_t threshold_num, double freq_boundary,
            double index_boundary) const;
    void MergeSampledThresholds(std::vector<double> const& less_thresholds,
                                std::vector<double> const& greater_thresholds,
                                model::ColumnIndex column_index);

public:
    ThresholdCalculator(
            std::shared_ptr<model::ColumnLayoutTypedRelationData> const& difference_typed_relation,
            std::shared_ptr<DistanceCalculator> distance_calculator, unsigned num_rows,
            model::ColumnIndex num_columns);

    std::vector<std::vector<model::DFConstraint>> const& GetThresholds() const noexcept {
        return thresholds_;
    }
};

}  // namespace algos::dd
