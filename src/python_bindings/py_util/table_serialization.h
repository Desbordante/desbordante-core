#pragma once

#include <memory>
#include <pybind11/pybind11.h>
#include "model/table/relational_schema.h"
#include "model/table/vertical.h"
#include "model/table/column.h"
#include "model/table/column_combination.h"

namespace py = pybind11;

namespace table_serialization {

py::tuple SerializeRelationalSchema(std::shared_ptr<RelationalSchema const> const& schema);

std::shared_ptr<RelationalSchema const> DeserializeRelationalSchema(py::tuple t);

py::tuple SerializeVertical(Vertical const& v);

Vertical DeserializeVertical(py::tuple t, std::shared_ptr<RelationalSchema const> schema);

py::tuple SerializeColumn(Column const& c);

Column DeserializeColumn(py::tuple t, std::shared_ptr<RelationalSchema const> schema);
}  // namespace table_serialization

