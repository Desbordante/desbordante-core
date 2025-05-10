#pragma once

#include <cstddef>
#include <functional>

#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "model/index.h"

namespace algos::hymde::cover_calculation::lattice {
using SingleLevelFunc = std::function<std::size_t(RecordClassifierValueId, model::Index)>;
}  // namespace algos::hymde::cover_calculation::lattice
