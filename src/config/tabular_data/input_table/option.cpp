#include "config/tabular_data/input_table/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kTable, descriptions::kDTable;
extern const CommonOption<InputTable> TableOpt{kTable, kDTable};
}  // namespace config
