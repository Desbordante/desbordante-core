#include "python_bindings/py_util/table_serialization.h"

#include <utility>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace table_serialization {

py::tuple SerializeRelationalSchema(RelationalSchema const* schema) {
    std::vector<std::unique_ptr<Column>> const& columns = schema->GetColumns();
    std::vector<std::pair<std::string, unsigned int>> col_data;
    col_data.reserve(columns.size());
    for (auto const& col_ptr : columns) {
        col_data.emplace_back(col_ptr->GetName(), col_ptr->GetIndex());
    }
    return py::make_tuple(schema->GetName(), std::move(col_data));
}

std::shared_ptr<RelationalSchema const> DeserializeRelationalSchema(py::tuple t) {
    if (t.size() != 2) {
        throw std::runtime_error("Invalid state for RelationalSchema pickle!");
    }
    auto schema_name = t[0].cast<std::string>();
    auto col_data = t[1].cast<std::vector<std::pair<std::string, unsigned int>>>();
    auto schema = std::make_shared<RelationalSchema>(std::move(schema_name));
    for (auto& [col_name, col_index] : col_data) {
        Column c(schema.get(), std::move(col_name), col_index);
        schema->AppendColumn(std::move(c));
    }
    return schema;
}

py::tuple SerializeVertical(Vertical const& v) {
    std::vector<unsigned int> idx_vec = v.GetColumnIndicesAsVector();
    return py::make_tuple(std::move(idx_vec));
}

Vertical DeserializeVertical(py::tuple t, RelationalSchema const* schema) {
    if (t.size() != 1) {
        throw std::runtime_error("Invalid state for Vertical pickle!");
    }
    auto idx_vec = t[0].cast<std::vector<unsigned int>>();
    boost::dynamic_bitset<> bitset = util::IndicesToBitset(idx_vec, schema->GetNumColumns());
    return Vertical(schema, std::move(bitset));
}

py::tuple SerializeColumn(Column const& c) {
    return py::make_tuple(c.GetName(), c.GetIndex());
}

Column DeserializeColumn(py::tuple t, RelationalSchema const* schema) {
    if (t.size() != 2) {
        throw std::runtime_error("Invalid state for Column pickle!");
    }
    auto col_name = t[0].cast<std::string>();
    auto col_index = t[1].cast<model::ColumnIndex>();
    Column col(schema, std::move(col_name), col_index);
    return col;
}
}  // namespace table_serialization
