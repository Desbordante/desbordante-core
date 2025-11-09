#include "dc/bind_adc_verification.h"

#include <pybind11/complex.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "algorithms/dc/ADCVerifier/adc_verifier.h"
#include "py_util/bind_primitive.h"

namespace python_bindings {

namespace py = pybind11;

void BindADCVerification(py::module_& main_module) {
    auto adc_verification_module = main_module.def_submodule("adc_verification");

    BindPrimitiveNoBase<algos::ADCVerifier>(adc_verification_module, "ADCVerification")
            .def("adc_holds", &algos::ADCVerifier::ADCHolds)
            .def("get_error", &algos::ADCVerifier::GetError)
            .def("get_violations", &algos::ADCVerifier::GetViolations);
}
}  // namespace python_bindings
