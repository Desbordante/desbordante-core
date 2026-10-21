#pragma once

#include <vector>

#include "core/model/table/idataset_stream.h"
#include "core/model/table/position_list_index.h"
#include "core/model/table/value_id_column.h"

namespace model {
std::vector<PositionListIndex> CreateStrippedPartitions(
        std::vector<ValueIdColumn> const& value_id_columns);

std::vector<PositionListIndex> CreateStrippedPartitions(IDatasetStream& data_stream);
}  // namespace model
