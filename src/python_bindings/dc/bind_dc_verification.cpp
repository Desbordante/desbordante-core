#include "dc/bind_dc_verification.h"

#include <pybind11/stl_bind.h>

#include "algorithms/dc/verifier/dc_verifier.h"
#include "py_util/bind_primitive.h"
#include "pybind11/complex.h"
#include "pybind11/stl.h"

PYBIND11_MAKE_OPAQUE(std::pair<size_t, size_t>)

// PYBIND11_MAKE_OPAQUE(std::vector<std::pair<size_t, size_t>>)

namespace python_bindings {

namespace py = pybind11;

void BindDCVerification(py::module_& main_module) {
    auto dc_verification_module = main_module.def_submodule("dc_verification");

    BindPrimitiveNoBase<algos::DCVerifier>(dc_verification_module, "DCVerification")
            .def("dc_holds", &algos::DCVerifier::DCHolds)
            .def("get_violations", &algos::DCVerifier::GetViolations);

    py::bind_vector<std::vector<std::pair<size_t, size_t>>>(main_module, "vector_pair");
}
}  // namespace python_bindings