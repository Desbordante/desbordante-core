#pragma once

#include <vector>

#include "core/model/table/idataset_stream.h"
#include "core/model/table/position_list_index.h"

namespace model {
std::vector<model::PositionListIndex> CreateStrippedPartitions(
        std::vector<std::vector<int>> const& value_id_mapped_table);

std::vector<model::PositionListIndex> CreateStrippedPartitions(model::IDatasetStream& data_stream);
}  // namespace model
