#include "config/tabular_data/table_modification_operations/update_old/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kUpdateOldStatements, descriptions::kDUpdateOldStatements;
extern CommonOption<std::pair<InputTable, InputTable>> const kUpdateOldStatementsOpt = {
        kUpdateOldStatements, kDUpdateOldStatements, std::nullopt, nullptr, nullptr};
}  // namespace config
