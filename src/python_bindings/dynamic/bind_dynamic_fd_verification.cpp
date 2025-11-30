#include "python_bindings/dynamic/bind_dynamic_fd_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/fd_verifier/dynamic_fd_verifier.h"
#include "core/algorithms/fd/verification_algorithms.h"
#include "core/config/names.h"
#include "core/config/tabular_data/crud_operations/operations.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/py_to_any.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindDynamicFdVerification(pybind11::module_& main_module) {
    using namespace algos;
    using namespace algos::fd_verifier;

    auto dynamic_fd_verification_module = main_module.def_submodule("dynamic_fd_verification");
    BindPrimitiveNoBase<DynamicFDVerifier>(dynamic_fd_verification_module, "DynamicFDVerifier")
            .def("fd_holds", &DynamicFDVerifier::FDHolds)
            .def("get_error", &DynamicFDVerifier::GetError)
            .def("get_num_error_clusters", &DynamicFDVerifier::GetNumErrorClusters)
            .def("get_highlights", &DynamicFDVerifier::GetHighlights);
}
}  // namespace python_bindings
