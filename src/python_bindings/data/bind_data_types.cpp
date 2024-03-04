#include "bind_data_types.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "config/tabular_data/input_table_type.h"
#include "model/table/column_combination.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindDataTypes(py::module_& main_module) {
    auto data_module = main_module.def_submodule("data_types");
    data_module.doc() = R"doc(
        Contains the types of data supported by Desbordante.

        Currently only used as tags for Algorithm.get_option_type
    )doc";
    py::class_<config::InputTable>(data_module, "Table");

    using namespace model;
    py::class_<ColumnCombination>(data_module, "ColumnCombination")
            .def("__str__", &ColumnCombination::ToString)
            .def_property_readonly("table_index", &ColumnCombination::GetTableIndex)
            .def_property_readonly("column_indices", &ColumnCombination::GetColumnIndices);
}
}  // namespace python_bindings
