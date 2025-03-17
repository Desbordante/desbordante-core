#pragma once

#include "config/tabular_data/input_table_type.h"  // for InputTable

namespace config {
template <typename T>
class CommonOption;
}

namespace config {
extern CommonOption<InputTable> const kInsertStatementsOpt;
}  // namespace config
