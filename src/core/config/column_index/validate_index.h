#pragma once

#include <cstddef>

#include "core/config/indices/type.h"
#include "core/config/column_index/type.h"

namespace config {
void ValidateIndex(IndexType value, size_t cols_count);
}  // namespace config
