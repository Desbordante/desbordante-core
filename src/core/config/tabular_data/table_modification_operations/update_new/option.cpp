#include "config/tabular_data/table_modification_operations/update_new/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kUpdateNewStatements, descriptions::kDUpdateNewStatements;
extern CommonOption<std::pair<InputTable, InputTable>> const kUpdateNewStatementsOpt = {
        kUpdateNewStatements, kDUpdateNewStatements, std::nullopt, nullptr, nullptr};
}  // namespace config
