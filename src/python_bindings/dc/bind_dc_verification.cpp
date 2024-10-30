#include "dc/bind_dc_verification.h"

#include "algorithms/dc/verifier/dc_verifier.h"
#include "py_util/bind_primitive.h"

namespace python_bindings {

namespace py = pybind11;

void BindDCVerification(py::module_& main_module) {
    auto dc_verification_module = main_module.def_submodule("dc_verification");

    BindPrimitiveNoBase<algos::DCVerifier>(dc_verification_module, "DCVerification")
            .def("dc_holds", &algos::DCVerifier::DCHolds);
}

}  // namespace python_bindings
