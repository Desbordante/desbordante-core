#pragma once

#include <cstddef>
#include <unordered_set>

#include "config/common_option.h"
#include "config/tabular_data/input_table_type.h"

namespace config {
extern CommonOption<std::unordered_set<size_t>> const kDeleteStatementsOpt;
}  // namespace config
