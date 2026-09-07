#pragma once

#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/model/table/position_list_index.h"

class ColumnOrder {
private:
    std::vector<int> order_;

public:
    explicit ColumnOrder(std::vector<model::PositionListIndex> const& input_table_column_plis);
    ColumnOrder() = default;

    std::vector<int> GetOrderHighDistinctCount(boost::dynamic_bitset<> const& columns) const;
    std::vector<int> GetOrderLowDistinctCount(boost::dynamic_bitset<> const& columns) const;
};
