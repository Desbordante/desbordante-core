#pragma once

#include <cstddef>

#include "algorithms/options/indices/type.h"

namespace algos::config {
void ValidateIndex(IndexType value, size_t cols_count);
}  // namespace algos::config
