#include "python_bindings/dc/bind_weever.h"

#include <pybind11/stl.h>

#include "core/algorithms/dc/weever/weever.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {

namespace py = pybind11;

void BindWeever(py::module_& main_module) {
    auto weever_module = main_module.def_submodule("weever");

    BindPrimitiveNoBase<algos::Weever>(weever_module, "Weever")
            .def("get_violations", &algos::Weever::GetViolations);
}

}  // namespace python_bindings
