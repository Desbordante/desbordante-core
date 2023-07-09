#pragma once

#include <cstddef>

#include "config/indices/type.h"

namespace config {
void ValidateIndex(IndexType value, size_t cols_count);
}  // namespace config
