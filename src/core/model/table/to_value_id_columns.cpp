#include "core/model/table/to_value_id_columns.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/model/index.h"
#include "core/util/logger.h"

namespace model {
std::vector<ValueIdColumn> ToValueIdColumns(IDatasetStream& data_stream) {
    std::unordered_map<std::string, int> value_id_map;
    int next_value_id = 0;
    std::size_t const num_columns = data_stream.GetNumberOfColumns();
    auto value_id_columns = std::vector<ValueIdColumn>(num_columns);
    std::vector<std::string> row;

    while (data_stream.HasNextRow()) {
        row = data_stream.GetNextRow();

        if (row.size() != num_columns) {
            LOG_WARN("Unexpected number of columns for a row, skipping (expected {}, got {})",
                     num_columns, row.size());
            continue;
        }

        for (Index column_index = 0; column_index != num_columns; ++column_index) {
            auto [it, is_new] =
                    value_id_map.try_emplace(std::move(row[column_index]), next_value_id);
            if (is_new) ++next_value_id;
            value_id_columns[column_index].push_back(it->second);
        }
    }

    return value_id_columns;
}
}  // namespace model
