#include "core/model/table/stripped_partitions.h"

#include "core/util/logger.h"

namespace model {
std::unique_ptr<StrippedPartitions> StrippedPartitions::CreateFrom(IDatasetStream& data_stream) {
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

        for (Index column_index = 0; column_index != num_columns; ++column_index) {
            std::string const& attribute_value = row[column_index];
            auto [it, is_new] = value_id_map.try_emplace(attribute_value, next_value_id);
            if (is_new) ++next_value_id;
            value_id_mapped_table[column_index].push_back(it->second);
        }
    }

    std::vector<PositionListIndex> stripped_partitions;
    for (Index column_index = 0; column_index != num_columns; ++column_index) {
        stripped_partitions.push_back(
                std::move(*PositionListIndex::CreateFor(value_id_mapped_table[column_index])));
    }

    return std::make_unique<StrippedPartitions>(std::move(stripped_partitions));
}
}  // namespace model
