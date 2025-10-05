#pragma once
#include <tuple>

#include "core/algorithms/cfd/cfdfinder/model/cfdfinder_relation_data.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder {
std::tuple<PLIs, Columns, Rows> Preprocess(CFDFinderRelationData* relation);
}  // namespace algos::cfdfinder
