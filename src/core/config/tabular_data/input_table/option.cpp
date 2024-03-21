#include "config/tabular_data/input_table/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kTable, descriptions::kDTable;
extern CommonOption<InputTable> const kTableOpt{kTable, kDTable};
}  // namespace config
