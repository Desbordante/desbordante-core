#pragma once

#include <cstddef>
#include <unordered_set>

#include "core/config/common_option.h"

namespace config {
extern CommonOption<std::unordered_set<size_t>> const kDeleteStatementsOpt;
}  // namespace config
