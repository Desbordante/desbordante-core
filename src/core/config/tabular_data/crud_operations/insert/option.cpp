#include "config/tabular_data/crud_operations/insert/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kInsertStatements, descriptions::kDInsertStatements;
extern CommonOption<InputTable> const kInsertStatementsOpt = {
        kInsertStatements, kDInsertStatements, std::nullopt, nullptr, nullptr};
}  // namespace config
