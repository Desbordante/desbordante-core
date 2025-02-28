#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/cfd/cfd_verifier/cfd_verifier.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindCFDVerification(py::module_& main_module) {
    using namespace algos;
    using namespace algos::cfd_verifier;

    auto cfd_verification_module = main_module.def_submodule("cfd_verification");

    BindPrimitiveNoBase<CFDVerifier>(cfd_verification_module, "CFDVerifier")
            .def("cfd_holds", &CFDVerifier::CFDHolds)
            .def("get_support", &CFDVerifier::GetSupport)
            .def("get_confidence", &CFDVerifier::GetConfidence)
            .def("get_rows_satisfying_cfd", &CFDVerifier::GetRowsSatisfyingCFD)
            .def("get_rows_violating_cfd", &CFDVerifier::GetRowsViolatingCFD);

    main_module.attr("cfd_verification") = cfd_verification_module;
}
}  // namespace python_bindings
