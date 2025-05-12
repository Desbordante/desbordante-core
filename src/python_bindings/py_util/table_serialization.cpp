#include "table_serialization.h"

#include <pybind11/stl.h>
#include <utility>

namespace py = pybind11;

namespace table_serialization {

py::tuple SerializeRelationalSchema(std::shared_ptr<RelationalSchema const> const& schema) {
    auto const& columns = schema->GetColumns();
    std::vector<std::pair<std::string, unsigned int>> col_data;
    col_data.reserve(columns.size());
    for (auto const& col_ptr : columns) {
        col_data.emplace_back(col_ptr->GetName(), col_ptr->GetIndex());
    }
    return py::make_tuple(schema->GetName(), col_data);
}

std::shared_ptr<RelationalSchema const> DeserializeRelationalSchema(py::tuple t) {
    if (t.size() != 2) {
        throw std::runtime_error("Invalid state for RelationalSchema pickle!");
    }
    auto schema_name = t[0].cast<std::string>();
    auto col_data = t[1].cast<std::vector<std::pair<std::string, unsigned int>>>();
    auto schema = std::make_shared<RelationalSchema>(schema_name);
    for (auto const& [col_name, col_index] : col_data) {
        Column c(schema.get(), col_name, col_index);
        schema->AppendColumn(std::move(c));
    }
    return schema;
}

py::tuple SerializeVertical(Vertical const& v) {
    auto idx_vec = v.GetColumnIndicesAsVector();
    return py::make_tuple(idx_vec);
}

Vertical DeserializeVertical(py::tuple t, std::shared_ptr<RelationalSchema const> schema) {
    if (t.size() != 1) {
        throw std::runtime_error("Invalid state for Vertical pickle!");
    }
    auto idx_vec = t[0].cast<std::vector<unsigned int>>();
    auto bitset = schema->IndicesToBitset(idx_vec);
    return Vertical(schema.get(), bitset);
}

py::tuple SerializeColumn(Column const& c) {
    return py::make_tuple(c.GetName(), c.GetIndex());
}

Column DeserializeColumn(py::tuple t, std::shared_ptr<RelationalSchema const> schema) {
    if (t.size() != 2) {
        throw std::runtime_error("Invalid state for Column pickle!");
    }
    auto col_name  = t[0].cast<std::string>();
    auto col_index = t[1].cast<model::ColumnIndex>();
    Column col(schema.get(), col_name, col_index);
    return col;
}
} // namespace table_serialization

