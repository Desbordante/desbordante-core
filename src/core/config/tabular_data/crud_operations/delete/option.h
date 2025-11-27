#pragma once

#include <cstddef>
#include <unordered_set>

#include "core/config/common_option.h"
#include "core/config/tabular_data/input_table_type.h"

namespace config {
extern CommonOption<std::unordered_set<size_t>> const kDeleteStatementsOpt;
}  // namespace config
