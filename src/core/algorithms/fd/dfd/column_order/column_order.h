#pragma once

#include "model/table/column_data.h"
#include "model/table/vertical.h"

class ColumnOrder {
private:
    std::vector<int> order_;

public:
    explicit ColumnOrder(ColumnLayoutRelationData const* const relation_data);
    ColumnOrder() = default;

    std::vector<int> GetOrderHighDistinctCount(Vertical const& columns) const;
    std::vector<int> GetOrderLowDistinctCount(Vertical const& columns) const;
};
