#pragma once

#include <tuple>

#include "model/column_layout_relation_data.h"
#include "types.h"

namespace algos::hyfd {

std::tuple<PLIs, Rows, std::vector<size_t>> Preprocess(ColumnLayoutRelationData *relation);

}  // namespace algos::hyfd
