#include "core/algorithms/fd/dfd/column_order/column_order.h"

#include <set>

#include "core/algorithms/fd/dfd/column_order/ordered_partition.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/relational_schema.h"

ColumnOrder::ColumnOrder(std::vector<model::PositionListIndex> const& input_table_column_plis)
    : order_(input_table_column_plis.size()) {
    std::set<OrderedPartition> partitions;
    for (model::Index column_index = 0; column_index != input_table_column_plis.size();
         ++column_index) {
        model::PositionListIndex const& pli = input_table_column_plis[column_index];
        partitions.emplace(&input_table_column_plis[column_index],
                           pli.GetCachedProbingTable()->size(), column_index);
    }

    int order_index = 0;
    for (auto const& partition : partitions) {
        order_[order_index++] = partition.GetColumnIndex();
    }
}

std::vector<int> ColumnOrder::GetOrderHighDistinctCount(
        boost::dynamic_bitset<> const& columns) const {
    std::vector<int> order_for_columns(columns.count());

    int current_order_index = 0;
    for (int column_index : order_) {
        if (columns.test(column_index)) {
            order_for_columns[current_order_index++] = column_index;
        }
    }

    return order_for_columns;
}

std::vector<int> ColumnOrder::GetOrderLowDistinctCount(
        boost::dynamic_bitset<> const& columns) const {
    std::vector<int> order_for_columns(columns.count());

    assert(!order_.empty());
    int current_order_index = 0;
    for (int i = this->order_.size() - 1; i >= 0; --i) {
        if (columns.test(order_[i])) {
            order_for_columns[current_order_index++] = this->order_[i];
        }
    }

    return order_for_columns;
}
