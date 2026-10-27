#pragma once

#include <memory>

#include "core/algorithms/fd/hycommon/types.h"

namespace algos::cfdfinder {
using PLIs = hy::PLIs;
using PLIsPtr = hy::PLIsPtr;
using Row = hy::Row;
using Rows = hy::Rows;
using Columns = hy::Columns;
using ColumnsPtr = std::shared_ptr<Columns>;
using RowsPtr = hy::RowsPtr;
}  // namespace algos::cfdfinder
