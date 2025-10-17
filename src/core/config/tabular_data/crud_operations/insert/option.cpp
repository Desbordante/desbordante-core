#include "config/tabular_data/crud_operations/insert/option.h"

#include <variant>

#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "descriptions.h"
#include "names.h"
#include "names_and_descriptions.h"
#include "tabular_data/input_table_type.h"

namespace config {
using names::kInsertStatements, descriptions::kDInsertStatements;
extern CommonOption<InputTable> const kInsertStatementsOpt = {kInsertStatements, kDInsertStatements,
                                                              InputTable{}};
}  // namespace config
