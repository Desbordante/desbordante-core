#include "python_bindings/cfd/bind_cfd_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/cfd/cfd_verifier/cfd_verifier.h"
#include "core/algorithms/cfd/cfd_verifier/highlight.h"
#include "core/algorithms/cfd/model/raw_cfd.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindCFDVerification(py::module_& main_module) {
    using namespace algos;
    using namespace algos::cfd;
    using namespace algos::cfd_verifier;

    auto cfd_verification_module = main_module.def_submodule("cfd_verification");

    py::class_<Highlight>(cfd_verification_module, "Highlight")
            .def_property_readonly("cluster", &Highlight::GetCluster)
            .def_property_readonly("violating_rows", &Highlight::GetViolatingRows);

    BindPrimitiveNoBase<CFDVerifier>(cfd_verification_module, "CFDVerifier")
            .def("cfd_holds", &CFDVerifier::CFDHolds)
            .def("get_real_support", &CFDVerifier::GetRealSupport)
            .def("get_real_confidence", &CFDVerifier::GetRealConfidence)
            .def("get_highlights", &CFDVerifier::GetHighlights)
            .def("get_num_clusters_violating_cfd", &CFDVerifier::GetNumClustersViolatingCFD)
            .def("get_num_rows_violating_cfd", &CFDVerifier::GetNumRowsViolatingCFD);

    main_module.attr("cfd_verification") = cfd_verification_module;
}
}  // namespace python_bindings
