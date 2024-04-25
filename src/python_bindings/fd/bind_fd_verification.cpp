#include "fd/bind_fd_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/fd/fd_verifier/fd_verifier.h"
#include "algorithms/fd/fd_verifier/highlight.h"
#include "algorithms/fd/verification_algorithms.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindFdVerification(pybind11::module_& main_module) {
    using namespace algos;
    using namespace algos::fd_verifier;

    auto fd_verification_module = main_module.def_submodule("fd_verification");
    py::class_<Highlight>(fd_verification_module, "Highlight")
            .def_property_readonly("cluster", &Highlight::GetCluster)
            .def_property_readonly("num_distinct_rhs_values", &Highlight::GetNumDistinctRhsValues)
            .def_property_readonly("most_frequent_rhs_value_proportion",
                                   &Highlight::GetMostFrequentRhsValueProportion);
    BindPrimitiveNoBase<FDVerifier>(fd_verification_module, "FDVerifier")
            .def("fd_holds", &FDVerifier::FDHolds)
            .def("get_error", &FDVerifier::GetError)
            .def("get_num_error_clusters", &FDVerifier::GetNumErrorClusters)
            .def("get_num_error_rows", &FDVerifier::GetNumErrorRows)
            .def("get_highlights", &FDVerifier::GetHighlights);

    main_module.attr("afd_verification") = fd_verification_module;
}
}  // namespace python_bindings
