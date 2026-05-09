#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include <boost/regex.hpp>

#include "core/algorithms/dd/dd.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"
#include "core/util/levenshtein_distance.h"

namespace algos::dd {

class DistanceCalculator {
private:
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    std::size_t const max_size_;
    std::vector<double> max_thresholds_;
    std::unique_ptr<unsigned[]> buf_;
    unsigned* r_buf_;

    std::size_t CalculateMaxStringSize() const {
        std::size_t max_size = 0;
        for (auto const& column : typed_relation_->GetColumnData()) {
            if (column.GetTypeId() == model::TypeId::kString) {
                for (auto value : column.GetData()) {
                    max_size =
                            std::max(max_size, model::Type::GetValue<model::String>(value).size());
                }
            }
        }
        return max_size;
    }

public:
    DistanceCalculator(std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation)
        : typed_relation_(typed_relation),
          max_size_(CalculateMaxStringSize() + 1),
          max_thresholds_(typed_relation_->GetColumnData().size(), static_cast<double>(max_size_)),
          buf_(std::make_unique<unsigned[]>(max_size_ * 2)),
          r_buf_(buf_.get() + max_size_) {}

    double CalculateDistance(model::ColumnIndex column_index,
                             std::pair<std::size_t, std::size_t> tuple_pair) const {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);

        double dif = 0;
        if (column.GetType().IsMetrizable()) {
            std::byte const* first_value = column.GetValue(tuple_pair.first);
            std::byte const* second_value = column.GetValue(tuple_pair.second);
            if (column.GetTypeId() == model::TypeId::kString) {
                auto const& type = column.GetType();
                auto first_ptr = &model::Type::GetValue<model::String>(first_value);
                auto second_ptr = &model::Type::GetValue<model::String>(second_value);
                std::size_t const max_dist = std::max(first_ptr->size(), second_ptr->size());
                dif = util::LevenshteinDistance(
                        first_ptr, second_ptr, buf_.get(), r_buf_,
                        static_cast<std::size_t>(std::ceil(max_thresholds_[column_index])),
                        max_dist);
            } else {
                auto const& type = static_cast<model::IMetrizableType const&>(column.GetType());
                dif = type.Dist(first_value, second_value);
            }
        }
        return dif;
    }

    void SetMaxThresholds(std::vector<std::vector<model::DFConstraint>> const& thresholds) {
        std::ranges::transform(
                thresholds, max_thresholds_, max_thresholds_.begin(),
                [](auto const& col_thresholds, double cur_max_threshold) {
                    return std::min(
                            cur_max_threshold,
                            std::ranges::max(col_thresholds, {}, [](auto const& constraint) {
                                return constraint.upper_bound;
                            }).upper_bound);
                });
    }

    model::CompareResult Compare(model::ColumnIndex column_index,
                                 std::pair<std::size_t, std::size_t> tuple_pair) const {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);

        std::byte const* first_value = column.GetValue(tuple_pair.first);
        std::byte const* second_value = column.GetValue(tuple_pair.second);
        auto const& type = static_cast<model::IMetrizableType const&>(column.GetType());

        return type.Compare(first_value, second_value);
    }

    bool IsDistanceOrdered(model::ColumnIndex column_index) const {
        return model::Type::IsDistanceOrdered(
                typed_relation_->GetColumnData(column_index).GetTypeId());
    }
};

}  // namespace algos::dd
