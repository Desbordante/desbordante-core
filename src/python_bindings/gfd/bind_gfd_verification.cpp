#include "gfd/bind_gfd_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/gfd/gfd_validator/verification_algorithms.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindGfdVerification(pybind11::module_& main_module) {
    using namespace algos;

    auto gfd_module = main_module.def_submodule("gfd_verification");

    py::class_<model::Gfd>(gfd_module, "Gfd").def("__str__", &model::Gfd::ToString);

    BindPrimitive<GfdValidator, EGfdValidator, NaiveGfdValidator>(
            gfd_module, &GfdHandler::GfdList, "GfdAlgorithm", "get_gfds",
            {"GfdValid", "EGfdValid", "NaiveGfdValid"}, py::return_value_policy::copy);
}
}  // namespace python_bindings
