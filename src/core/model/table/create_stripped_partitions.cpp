#include "core/model/table/create_stripped_partitions.h"

#include <utility>

#include "core/model/table/to_value_id_mapped_columns.h"

namespace model {
std::vector<model::PositionListIndex> CreateStrippedPartitions(
        std::vector<std::vector<int>> const& value_id_mapped_table) {
    std::vector<model::PositionListIndex> stripped_partitions;
    for (std::vector<int> const& mapped_column : value_id_mapped_table) {
        stripped_partitions.push_back(
                std::move(*model::PositionListIndex::CreateFor(mapped_column)));
    }
    return stripped_partitions;
}

std::vector<model::PositionListIndex> CreateStrippedPartitions(model::IDatasetStream& data_stream) {
    return CreateStrippedPartitions(ToValueIdMappedColumns(data_stream));
}
}  // namespace model
