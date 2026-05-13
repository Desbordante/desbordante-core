#include "core/config/tabular_data/input_table/option.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "core/config/names_and_descriptions.h"
#include "core/config/exceptions.h"

namespace config {
using names::kTable, descriptions::kDTable;
extern CommonOption<InputTable> const kTableOpt = {
        kTable, kDTable, std::nullopt, nullptr, [](InputTable const& table) {
            // Shouldn't happen normally.
            if (table == nullptr) throw ConfigurationError("Table must be a valid pointer!");
        }};
}  // namespace config
