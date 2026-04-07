#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/fastdd/model/differential_function.h"
#include "core/algorithms/dd/fastdd/util/distance_calculator.h"
#include "core/model/table/column_index.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/table/typed_column_data.h"
#include "core/model/types/type.h"

namespace algos::dd {

struct ThresholdInfo {
    double threshold;
    bool is_upper_bound;
    std::size_t dif_func_index;

    auto operator<=>(ThresholdInfo const& other) const {
        if (!model::IsEqual(threshold, other.threshold)) {
            return model::Less(threshold, other.threshold) ? std::strong_ordering::less
                                                           : std::strong_ordering::greater;
        }
        if (is_upper_bound != other.is_upper_bound) {
            return is_upper_bound < other.is_upper_bound ? std::strong_ordering::less
                                                         : std::strong_ordering::greater;
        }
        if (dif_func_index != other.dif_func_index) {
            if (!is_upper_bound) {
                return dif_func_index < other.dif_func_index ? std::strong_ordering::less
                                                             : std::strong_ordering::greater;
            } else {
                return dif_func_index > other.dif_func_index ? std::strong_ordering::less
                                                             : std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::equal;
    }

    bool operator==(ThresholdInfo const& other) const {
        return model::IsEqual(threshold, other.threshold) &&
               is_upper_bound == other.is_upper_bound && dif_func_index == other.dif_func_index;
    }
};

class DifferentialFunctionBuilder {
private:
    std::vector<std::vector<DifferentialFunction>> differential_functions_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    unsigned num_rows_;
    model::ColumnIndex num_columns_;
    std::shared_ptr<DistanceCalculator> distance_calculator_;
    std::vector<model::DFConstraint> min_max_dif_;

    std::vector<std::vector<ThresholdInfo>> thresholds_;
    std::vector<std::vector<std::size_t>> threshold_zones_;
    std::vector<std::vector<boost::dynamic_bitset<>>> zone_to_bitset_;
    std::vector<std::size_t> dif_func_nums_;

    std::vector<DifferentialFunction> GetThresholds(model::TypedColumnData const& dif_column,
                                                    model::ColumnIndex column_index) const;

    std::pair<std::vector<double>, std::vector<double>> SampleThresholds(
            model::ColumnIndex const column_index, std::vector<std::size_t> const& row_nums,
            std::size_t row_limit, std::size_t threshold_num, double freq_boundary,
            double index_boundary) const;
    std::vector<std::size_t> SampleRows(std::size_t row_limit) const;

    void AddThresholds(std::vector<double> const& less_thresholds,
                       std::vector<double> const& greater_thresholds,
                       model::ColumnIndex const column_index);

    void CalculateThresholdZones();

public:
    DifferentialFunctionBuilder(
            std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation, unsigned num_rows,
            model::ColumnIndex num_columns, std::shared_ptr<DistanceCalculator> distance_calculator,
            std::vector<model::DFConstraint> min_max_dif)
        : typed_relation_(typed_relation),
          num_rows_(num_rows),
          num_columns_(num_columns),
          distance_calculator_(distance_calculator),
          min_max_dif_(std::move(min_max_dif)) {}

    void BuildDFList(
            std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation);

    std::vector<ThresholdInfo> const& GetThresholds(model::ColumnIndex const column_index) const {
        return thresholds_[column_index];
    }

    std::vector<std::size_t> const& GetThresholdZones(model::ColumnIndex const column_index) const {
        return threshold_zones_[column_index];
    }

    std::vector<std::vector<DifferentialFunction>> const& GetDifFuncs() const noexcept {
        return differential_functions_;
    }

    std::size_t GetDifFuncsSize() const noexcept {
        return differential_functions_.size();
    }

    std::size_t GetDifFuncNum() const noexcept {
        return dif_func_nums_[dif_func_nums_.size() - 1];
    }

    bool IsDistanceOrdered(model::ColumnIndex const column_index) const {
        return model::Type::IsDistanceOrdered(
                typed_relation_->GetColumnData(column_index).GetTypeId());
    }

    template <typename Bitset>
    std::vector<Bitset> GetSatisfiedDFs(model::ColumnIndex const column_index) const {
        return std::vector<Bitset>(zone_to_bitset_[column_index].begin(),
                                   zone_to_bitset_[column_index].end());
    }

    std::size_t GetZoneNum(model::ColumnIndex const column_index) const {
        return zone_to_bitset_[column_index].size();
    }
};

}  // namespace algos::dd
