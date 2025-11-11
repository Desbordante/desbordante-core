#pragma once

#include <memory>

#include "algorithms/cfd/cfdfinder/model/cfdfinder_relation_data.h"
#include "algorithms/fd/hycommon/types.h"

namespace algos::cfdfinder {
using ColumnsPtr = std::shared_ptr<hy::Columns>;
std::tuple<hy::PLIsPtr, ColumnsPtr, hy::RowsPtr> Preprocess(CFDFinderRelationData* relation);
}  // namespace algos::cfdfinder
