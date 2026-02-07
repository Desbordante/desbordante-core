#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/config/indices/type.h"
#include "core/model/table/typed_column_data.h"

namespace pac::util {
inline std::shared_ptr<model::Tuples> MakeTuples(
        std::vector<::model::TypedColumnData> const& col_data, config::IndicesType const& indices) {
    std::vector<std::vector<std::byte const*> const*> columns_data;
    columns_data.reserve(indices.size());
    std::ranges::transform(
            indices, std::back_inserter(columns_data),
            [&col_data](auto const col_idx) { return &col_data[col_idx].GetData(); });

    auto num_rows = col_data.front().GetNumRows();
    auto tuples = std::make_shared<model::Tuples>(num_rows);
    for (std::size_t row_idx = 0; row_idx < num_rows; ++row_idx) {
        auto& tuple = (*tuples)[row_idx];
        tuple = model::Tuple(indices.size());
        std::ranges::transform(columns_data, tuple.begin(),
                               [row_idx](auto const* col_data) { return (*col_data)[row_idx]; });
    }
    return tuples;
}
}  // namespace pac::util
