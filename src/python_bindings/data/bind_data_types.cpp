#include "python_bindings/data/bind_data_types.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_combination.h"

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
            .def("to_index_tuple",
                 [](ColumnCombination const& cc) {
                     return py::make_tuple(cc.GetTableIndex(), py::cast(cc.GetColumnIndices()));
                 })
            .def_property_readonly("table_index", &ColumnCombination::GetTableIndex)
            .def_property_readonly("column_indices", &ColumnCombination::GetColumnIndices)
            .def(py::pickle(
                    // __getstate__
                    [](ColumnCombination const& cc) {
                        return py::make_tuple(cc.GetTableIndex(), cc.GetColumnIndices());
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for ColumnCombination pickle!");
                        }
                        auto table_index = t[0].cast<model::ColumnIndex>();
                        auto col_indices = t[1].cast<std::vector<model::ColumnIndex>>();
                        return ColumnCombination(table_index, col_indices);
                    }));
}
}  // namespace python_bindings
