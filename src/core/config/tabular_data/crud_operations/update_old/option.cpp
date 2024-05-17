#include "config/tabular_data/crud_operations/update_old/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kUpdateOldStatements, descriptions::kDUpdateOldStatements;
extern CommonOption<std::vector<size_t>> const kUpdateOldStatementsOpt = {
        kUpdateOldStatements, kDUpdateOldStatements, std::nullopt, nullptr, nullptr};
}  // namespace config
