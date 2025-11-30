// This option is meant for the common case where an algorithm accepts only one
// table.

#pragma once

#include "core/config/common_option.h"
#include "core/config/tabular_data/input_table_type.h"

namespace config {
extern CommonOption<InputTable> const kTableOpt;
}  // namespace config
