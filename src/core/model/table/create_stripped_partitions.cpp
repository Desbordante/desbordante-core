#include "core/model/table/create_stripped_partitions.h"

#include <utility>

#include "core/model/table/to_value_id_columns.h"

namespace model {
std::vector<PositionListIndex> CreateStrippedPartitions(
        std::vector<ValueIdColumn> const& value_id_columns) {
    std::vector<PositionListIndex> stripped_partitions;
    for (ValueIdColumn const& value_id_column : value_id_columns) {
        stripped_partitions.push_back(std::move(*PositionListIndex::CreateFor(value_id_column)));
    }
    return stripped_partitions;
}

std::vector<PositionListIndex> CreateStrippedPartitions(IDatasetStream& data_stream) {
    return CreateStrippedPartitions(ToValueIdColumns(data_stream));
}
}  // namespace model
