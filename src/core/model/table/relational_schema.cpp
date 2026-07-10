#include "core/model/table/relational_schema.h"

#include <memory>
#include <utility>

#include "core/model/table/vertical.h"
#include "core/model/table/vertical_map.h"

namespace {
std::vector<std::unique_ptr<Column>> MakeColumns(RelationalSchema const* schema,
                                                 std::vector<std::string> column_names) {
    std::vector<std::unique_ptr<Column>> columns;
    std::size_t const number_of_columns = column_names.size();
    columns.reserve(number_of_columns);
    for (model::ColumnIndex i = 0; i != number_of_columns; ++i) {
        columns.push_back(std::make_unique<Column>(schema, std::move(column_names[i]), i));
    }
    return columns;
}
}  // namespace

RelationalSchema::RelationalSchema(std::string name, std::vector<std::string> column_names)
    : columns_(MakeColumns(this, std::move(column_names))), name_(std::move(name)) {}

std::unique_ptr<RelationalSchema> RelationalSchema::CreateFrom(model::IDatasetStream& table) {
    std::size_t const number_of_columns = table.GetNumberOfColumns();
    std::vector<std::string> column_names;
    column_names.reserve(number_of_columns);
    for (model::ColumnIndex i = 0; i != number_of_columns; ++i) {
        column_names.push_back(table.GetColumnName(i));
    }
    return std::make_unique<RelationalSchema>(table.GetRelationName(), std::move(column_names));
}

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

size_t RelationalSchema::GetNumColumns() const {
    return columns_.size();
}

RelationalSchema::~RelationalSchema() = default;
