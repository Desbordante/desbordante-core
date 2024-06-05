#include "data/bind_data.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "config/tabular_data/input_table_type.h"
#include "model/table/column_combination.h"

namespace py = pybind11;

namespace python_bindings {
void BindDataModule(py::module_& main_module) {
    auto data_module = main_module.def_submodule("data");
    data_module.doc() = "Contains everything related to data itself.";
    auto table_tag = py::class_<config::InputTable>(data_module, "Table");
    table_tag.doc() = "Tag type for tabular data.";

    using namespace model;
    py::class_<ColumnCombination>(data_module, "ColumnCombination")
            .def("__str__", &ColumnCombination::ToString)
            .def_property_readonly("table_index", &ColumnCombination::GetTableIndex)
            .def_property_readonly("column_indices", &ColumnCombination::GetColumnIndices);
}
}  // namespace python_bindings
