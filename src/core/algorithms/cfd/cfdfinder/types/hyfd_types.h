#pragma once

#include <memory>
#include <vector>

#include "core/model/table/column_index.h"
#include "core/model/table/position_list_index.h"

namespace algos::cfdfinder {
using PLIs = std::vector<model::PositionListIndex*>;
using PLIsPtr = std::shared_ptr<PLIs>;
using Row = std::vector<model::ColumnIndex>;
using Rows = std::vector<Row>;
using Columns = std::vector<std::vector<model::ColumnIndex>>;
using ColumnsPtr = std::shared_ptr<Columns>;
using RowsPtr = std::shared_ptr<Rows>;
}  // namespace algos::cfdfinder
