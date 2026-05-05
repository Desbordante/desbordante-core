#pragma once

#include <memory>

#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"

namespace algos::dd {

class DistanceCalculator {
private:
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

public:
    DistanceCalculator(std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation)
        : typed_relation_(typed_relation) {}

    double CalculateDistance(model::ColumnIndex column_index,
                             std::pair<std::size_t, std::size_t> tuple_pair) const {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);

        double dif = 0;
        if (column.GetType().IsMetrizable()) {
            std::byte const* first_value = column.GetValue(tuple_pair.first);
            std::byte const* second_value = column.GetValue(tuple_pair.second);
            auto const& type = static_cast<model::IMetrizableType const&>(column.GetType());
            dif = type.Dist(first_value, second_value);
        }
        return dif;
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
