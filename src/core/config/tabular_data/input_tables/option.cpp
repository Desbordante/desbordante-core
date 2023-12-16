#include "config/tabular_data/input_tables/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kTables, descriptions::kDTables;
extern CommonOption<InputTables> const TablesOpt{kTables, kDTables};
}  // namespace config
