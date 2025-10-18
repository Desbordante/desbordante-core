#include "config/tabular_data/input_table/option.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "exceptions.h"
#include "tabular_data/input_table_type.h"

namespace config {
using names::kTable, descriptions::kDTable;
extern CommonOption<InputTable> const kTableOpt = {
        kTable, kDTable, std::nullopt, nullptr, [](InputTable const& table) {
            // Shouldn't happen normally.
            if (table == nullptr) throw ConfigurationError("Table must be a valid pointer!");
        }};
}  // namespace config
