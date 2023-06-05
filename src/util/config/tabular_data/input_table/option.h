// This option is meant for the common case where an algorithm accepts only one
// table.

#pragma once

#include "util/config/common_option.h"
#include "util/config/tabular_data/input_table_type.h"

namespace util::config {
extern const CommonOption<InputTable> TableOpt;
}  // namespace util::config
