#pragma once

#include "algorithms/cfd/cfdfinder/model/cfdfinder_relation_data.h"
#include "algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder {
std::tuple<PLIs, Columns, Rows> Preprocess(CFDFinderRelationData* relation);
}  // namespace algos::cfdfinder
