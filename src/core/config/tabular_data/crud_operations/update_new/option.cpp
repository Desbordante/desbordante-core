#include "config/tabular_data/crud_operations/update_new/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kUpdateNewStatements, descriptions::kDUpdateNewStatements;
extern CommonOption<InputTable> const kUpdateNewStatementsOpt = {
        kUpdateNewStatements, kDUpdateNewStatements, std::nullopt, nullptr, nullptr};
}  // namespace config
