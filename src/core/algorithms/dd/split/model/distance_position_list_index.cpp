#include "algorithms/dd/split/model/distance_position_list_index.h"

#include <string>
#include <utility>

#include "model/table/tuple_index.h"
#include "model/table/typed_column_data.h"

namespace algos::dd {
template <typename T>
void DistancePositionListIndex::AddValue(T&& value) {
    auto&& [it, is_value_new] =
            value_mapping_.try_emplace(std::forward<T>(value), next_cluster_index_);
    if (is_value_new) {
        clusters_.emplace_back(cur_tuple_index_, 0);
        ++next_cluster_index_;
    }
    ++clusters_[it->second].size;
    inverted_index_.push_back(it->second);
    ++cur_tuple_index_;
}

DistancePositionListIndex::DistancePositionListIndex(model::TypedColumnData const& column,
                                                     model::TupleIndex num_rows) {
    if (num_rows == 0) num_rows = column.GetNumRows();
    clusters_.reserve(num_rows);
    inverted_index_.reserve(num_rows);
    for (model::TupleIndex index = 0; index != num_rows; ++index) {
        AddValue(column.GetDataAsString(index));
    }
}
}  // namespace algos::dd
