#include "config/tabular_data/crud_operations/update/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kUpdateStatements, descriptions::kDUpdateStatements;
extern CommonOption<InputTable> const kUpdateStatementsOpt = {kUpdateStatements, kDUpdateStatements,
                                                              InputTable{}};
}  // namespace config
