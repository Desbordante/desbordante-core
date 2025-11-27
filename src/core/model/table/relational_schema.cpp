#include "core/model/table/relational_schema.h"

#include <memory>
#include <utility>

#include "core/model/table/vertical.h"
#include "core/model/table/vertical_map.h"

RelationalSchema::RelationalSchema(std::string name) : columns_(), name_(std::move(name)) {}

Vertical RelationalSchema::GetVertical(boost::dynamic_bitset<> indices) const {
    return {this, std::move(indices)};
}

Vertical RelationalSchema::CreateEmptyVertical() const {
    return {this, boost::dynamic_bitset<>(GetNumColumns())};
}

bool RelationalSchema::IsColumnInSchema(std::string const& col_name) const {
    return std::find_if(columns_.begin(), columns_.end(), [&col_name](auto& column) {
               return column->GetName() == col_name;
           }) != columns_.end();
}

Column const* RelationalSchema::GetColumn(std::string const& col_name) const {
    auto found_entry_iterator =
            std::find_if(columns_.begin(), columns_.end(),
                         [&col_name](auto& column) { return column->GetName() == col_name; });
    if (found_entry_iterator != columns_.end()) return found_entry_iterator->get();

    throw std::invalid_argument("Couldn't match column name \'" + col_name +
                                "\' to any of the schema's column names");
}

Column const* RelationalSchema::GetColumn(size_t index) const {
    return columns_.at(index).get();
}

void RelationalSchema::AppendColumn(std::string const& col_name) {
    columns_.push_back(std::make_unique<Column>(this, col_name, columns_.size()));
}

void RelationalSchema::AppendColumn(Column column) {
    columns_.push_back(std::make_unique<Column>(std::move(column)));
}

size_t RelationalSchema::GetNumColumns() const {
    return columns_.size();
}

RelationalSchema::~RelationalSchema() = default;
