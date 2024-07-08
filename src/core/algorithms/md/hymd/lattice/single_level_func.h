#pragma once

#include <cstddef>
#include <functional>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "model/index.h"

namespace algos::hymd::lattice {
using SingleLevelFunc = std::function<std::size_t(ColumnClassifierValueId, model::Index)>;
}  // namespace algos::hymd::lattice
