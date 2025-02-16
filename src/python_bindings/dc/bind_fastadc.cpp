#include "bind_fastadc.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/dc/FastADC/fastadc.h"
#include "py_util/bind_primitive.h"

namespace py = pybind11;

namespace python_bindings {
void BindFastADC(py::module_& main_module) {
    using namespace algos;
    using DC = algos::fastadc::DenialConstraint;

    auto dc_module = main_module.def_submodule("dc");
    py::class_<DC>(dc_module, "DC").def("__str__", &DC::ToString).def("__repr__", &DC::ToString);

    BindPrimitiveNoBase<dc::FastADC>(dc_module, "FastADC").def("get_dcs", &dc::FastADC::GetDCs);
}
}  // namespace python_bindings
