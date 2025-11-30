#pragma once

#include <cstddef>
#include <functional>

#include "core/algorithms/md/hymd/column_classifier_value_id.h"
#include "core/model/index.h"

namespace algos::hymd::lattice {
using SingleLevelFunc = std::function<std::size_t(ColumnClassifierValueId, model::Index)>;
}  // namespace algos::hymd::lattice
