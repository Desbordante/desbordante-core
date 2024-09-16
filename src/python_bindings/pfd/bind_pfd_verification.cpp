#include "bind_pfd_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/fd/pfd_verifier/pfd_verifier.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindPfdVerification(py::module_& main_module) {
    using namespace algos;
    auto pfd_verification_module = main_module.def_submodule("pfd_verification");

    BindPrimitiveNoBase<PFDVerifier>(pfd_verification_module, "PFDVerifier")
            .def("get_num_violating_clusters", &PFDVerifier::GetNumViolatingClusters)
            .def("get_num_violating_rows", &PFDVerifier::GetNumViolatingRows)
            .def("get_violating_clusters", &PFDVerifier::GetViolatingClusters)
            .def("get_error", &PFDVerifier::GetError);
    main_module.attr("pfd_verification") = pfd_verification_module;
}
}  // namespace python_bindings
