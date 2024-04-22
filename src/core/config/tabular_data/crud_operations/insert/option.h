// This option is meant for the dynamic case where an algorithm processes crud
// operations over table many times.

#pragma once

#include "config/common_option.h"
#include "config/tabular_data/input_table_type.h"

namespace config {
extern CommonOption<InputTable> const kInsertStatementsOpt;
}  // namespace config
