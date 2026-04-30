#include "core/model/table/create_stripped_partitions.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/model/index.h"
#include "core/util/logger.h"

namespace util {
std::vector<std::vector<int>> CreateValueIdMap(model::IDatasetStream& data_stream) {
    std::unordered_map<std::string, int> value_id_map;
    int next_value_id = 0;
    std::size_t const num_columns = data_stream.GetNumberOfColumns();
    auto value_id_mapped_table = std::vector<std::vector<int>>(num_columns);
    std::vector<std::string> row;

    while (data_stream.HasNextRow()) {
        row = data_stream.GetNextRow();

        if (row.size() != num_columns) {
            LOG_WARN("Unexpected number of columns for a row, skipping (expected {}, got {})",
                     num_columns, row.size());
            continue;
        }

        for (model::Index column_index = 0; column_index != num_columns; ++column_index) {
            std::string const& attribute_value = row[column_index];
            auto [it, is_new] = value_id_map.try_emplace(attribute_value, next_value_id);
            if (is_new) ++next_value_id;
            value_id_mapped_table[column_index].push_back(it->second);
        }
    }

    return value_id_mapped_table;
}

std::vector<model::PositionListIndex> CreateStrippedPartitions(
        std::vector<std::vector<int>> const& value_id_mapped_table) {
    std::vector<model::PositionListIndex> stripped_partitions;
    for (std::vector<int> const& column_map : value_id_mapped_table) {
        stripped_partitions.push_back(std::move(*model::PositionListIndex::CreateFor(column_map)));
    }
    return stripped_partitions;
}
}  // namespace util
