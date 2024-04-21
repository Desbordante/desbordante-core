// This option is meant for the common case where an algorithm accepts only one
// table.

#pragma once

#include "config/common_option.h"
#include "config/tabular_data/input_table_type.h"

namespace config {
extern CommonOption<InputTable> const kInsertStatementsOpt;
}  // namespace config
