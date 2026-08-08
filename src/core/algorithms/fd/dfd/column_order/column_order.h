#pragma once

#include "core/model/table/column_data.h"

class ColumnOrder {
private:
    std::vector<int> order_;

public:
    explicit ColumnOrder(ColumnLayoutRelationData const* const relation_data);
    ColumnOrder() = default;

    std::vector<int> GetOrderHighDistinctCount(boost::dynamic_bitset<> const& columns) const;
    std::vector<int> GetOrderLowDistinctCount(boost::dynamic_bitset<> const& columns) const;
};
