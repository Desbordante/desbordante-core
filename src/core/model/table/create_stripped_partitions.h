#pragma once

#include <vector>

#include "core/model/table/idataset_stream.h"
#include "core/model/table/position_list_index.h"

namespace util {
std::vector<std::vector<int>> CreateValueIdMap(model::IDatasetStream& data_stream);
std::vector<model::PositionListIndex> CreateStrippedPartitions(
        std::vector<std::vector<int>> const& value_id_mapped_table);

inline std::vector<model::PositionListIndex> CreateStrippedPartitions(model::IDatasetStream& data_stream) {
    return CreateStrippedPartitions(CreateValueIdMap(data_stream));
}
}  // namespace util
