#pragma once

#include <cstddef>

#include "util/config/indices/type.h"

namespace util::config {
void ValidateIndex(IndexType value, size_t cols_count);
}  // namespace util::config
