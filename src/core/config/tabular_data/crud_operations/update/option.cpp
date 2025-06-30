#include "config/tabular_data/crud_operations/update/option.h"

#include <variant>  // for variant

#include "common_option.h"  // for CommonOption
#include "config/names_and_descriptions.h"
#include "tabular_data/input_table_type.h"  // for InputTable

namespace config {
using names::kUpdateStatements, descriptions::kDUpdateStatements;
extern CommonOption<InputTable> const kUpdateStatementsOpt = {kUpdateStatements, kDUpdateStatements,
                                                              InputTable{}};
}  // namespace config
