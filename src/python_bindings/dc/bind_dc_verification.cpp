#include "python_bindings/dc/bind_dc_verification.h"

#include <pybind11/complex.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "core/algorithms/dc/verifier/dc_verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {

namespace py = pybind11;

void BindDCVerification(py::module_& main_module) {
    auto dc_verification_module = main_module.def_submodule("dc_verification");

    BindPrimitiveNoBase<algos::DCVerifier>(dc_verification_module, "DCVerification")
            .def("dc_holds", &algos::DCVerifier::DCHolds)
            .def("get_violations", &algos::DCVerifier::GetViolations);
}
}  // namespace python_bindings
