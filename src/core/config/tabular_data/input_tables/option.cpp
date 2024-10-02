#include "config/tabular_data/input_tables/option.h"

#include <cassert>

#include "config/names_and_descriptions.h"

namespace config {

InputTablesOption::InputTablesOption(std::string_view name, std::string_view description)
    : common_option_(name, description) {}

std::string_view InputTablesOption::GetName() const {
    return common_option_.GetName();
}

Option<InputTables> InputTablesOption::operator()(InputTables* value_ptr,
                                                  TableIndex tables_limit) const {
    assert(tables_limit != 0);
    Option<InputTables> option = common_option_(value_ptr);
    option.SetValueCheck([tables_limit](InputTables const& tables) {
        if (tables.empty()) {
            throw ConfigurationError("Invalid input: expected at least one table");
        }

        if (tables.size() > tables_limit) {
            throw ConfigurationError("Invalid input: expected at most " +
                                     std::to_string(tables_limit) + " tables, but received " +
                                     std::to_string(tables.size()));
        }
    });

    return option;
}

using names::kTables, descriptions::kDTables;
extern InputTablesOption const kTablesOpt{kTables, kDTables};
}  // namespace config
