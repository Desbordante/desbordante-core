#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/algorithms/dd/fastdd/model/differential_function.h"
#include "core/algorithms/dd/fastdd/providers/df_provider.h"
#include "core/algorithms/dd/fastdd/providers/index_provider.h"
#include "core/algorithms/dd/fastdd/util/distance_calculator.h"
#include "core/model/table/column_index.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/table/typed_column_data.h"
#include "core/model/types/type.h"

namespace algos::dd {

class DifferentialFunctionBuilder {
private:
    std::vector<std::vector<DifferentialFunction>> differential_functions_;
    DFProvider df_provider_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    unsigned num_rows_;
    model::ColumnIndex num_columns_;
    std::shared_ptr<DistanceCalculator> distance_calculator_;

    std::vector<std::vector<double>> thresholds_;
    std::vector<std::size_t> dif_func_nums_;

    std::pair<std::vector<double>, std::vector<double>> GetThresholds(
            model::TypedColumnData const& dif_column) const;

    std::pair<std::vector<double>, std::vector<double>> SampleThresholds(
            model::ColumnIndex const column_index, std::vector<std::size_t> const& row_nums,
            std::size_t row_limit, std::size_t threshold_num, double freq_boundary,
            double index_boundary) const;
    std::vector<std::size_t> SampleRows(std::size_t row_limit) const;

    void AddThresholds(std::vector<double> const& less_thresholds,
                       std::vector<double> const& greater_thresholds,
                       model::ColumnIndex const column_index);

public:
    DifferentialFunctionBuilder(
            std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation, unsigned num_rows,
            model::ColumnIndex num_columns, std::shared_ptr<DistanceCalculator> distance_calculator)
        : typed_relation_(typed_relation),
          num_rows_(num_rows),
          num_columns_(num_columns),
          distance_calculator_(distance_calculator),
          thresholds_(num_columns) {}

    void BuildDFList(
            std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation);

    std::vector<double> GetThresholds(model::ColumnIndex const column_index) const {
        return thresholds_[column_index];
    }

    std::vector<std::vector<DifferentialFunction>> GetDifFuncs() const noexcept {
        return differential_functions_;
    }

    model::ColumnIndex GetNumColumns() const noexcept {
        return num_columns_;
    }

    std::size_t GetDifFuncNum() const noexcept {
        return dif_func_nums_[dif_func_nums_.size() - 1];
    }

    bool IsDistanceOrdered(model::ColumnIndex const column_index) const {
        return model::Type::IsDistanceOrdered(
                typed_relation_->GetColumnData(column_index).GetTypeId());
    }

    std::vector<boost::dynamic_bitset<>> GetSatisfiedDFs(
            model::ColumnIndex const column_index) const;
};

}  // namespace algos::dd
