#pragma once

#include "algorithms/cfd/cfdfinder/model/cfdfinder_relation_data.h"
#include "algorithms/fd/hycommon/types.h"

namespace algos::cfdfinder {
std::tuple<hy::PLIs, hy::Columns, hy::Rows> Preprocess(CFDFinderRelationData* relation);
}  // namespace algos::cfdfinder
