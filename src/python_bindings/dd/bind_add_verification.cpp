#include "python_bindings/dd/bind_add_verification.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/dd/add_verifier/add_verifier.h"
#include "core/algorithms/dd/dd_verifier/dd_verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {
namespace py = pybind11;

void BindADDVerification(py::module_& main_module) {
    using namespace algos;
    auto add_verification_module = main_module.def_submodule("add_verification");

    BindPrimitiveNoBase<dd::ADDVerifier>(add_verification_module, "ADDVerifier")
            .def("dd_holds", &dd::ADDVerifier::DDHolds)
            .def("get_error", &dd::ADDVerifier::GetError)
            .def("get_num_error_pairs", &dd::ADDVerifier::GetNumErrorRhs)
            .def("get_highlights", &dd::ADDVerifier::GetHighlights);
}
}  // namespace python_bindings
