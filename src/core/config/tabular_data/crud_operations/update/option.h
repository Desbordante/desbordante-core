#pragma once

#include "config/common_option.h"
#include "config/tabular_data/input_table_type.h"

namespace config {
template <typename T>
class CommonOption;

extern CommonOption<InputTable> const kUpdateStatementsOpt;
}  // namespace config
