#pragma once

#include <utility>

#include "util/desbordante_assume.h"

namespace algos::hymde::utility {
template <typename T>
T& NotEmpty(T& range) {
    DESBORDANTE_ASSUME(range.begin() != range.end());
    return range;
}
}  // namespace algos::hymde::utility
