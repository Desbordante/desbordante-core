#include "bind_cind_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/cind/cind_verifier/cind_verifier.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {

void BindCindVerification(py::module_& main_module) {
    using namespace algos::cind;

    auto cind_verification_module = main_module.def_submodule("cind_verification");

    using ViolatingCluster = CINDVerifier::ViolatingCluster;

    py::class_<ViolatingCluster>(cind_verification_module, "ViolatingCluster")
            .def_readonly("basket_rows", &ViolatingCluster::basket_rows)
            .def_readonly("violating_rows", &ViolatingCluster::violating_rows);

    BindPrimitiveNoBase<CINDVerifier>(cind_verification_module, "CINDVerifier")
            .def("holds", &CINDVerifier::Holds)
            .def("get_real_validity", &CINDVerifier::GetRealValidity)
            .def("get_real_completeness", &CINDVerifier::GetRealCompleteness)
            .def("get_violating_rows_count", &CINDVerifier::GetViolatingRowsCount)
            .def("get_violating_clusters_count", &CINDVerifier::GetViolatingClustersCount)
            .def("get_violating_clusters", &CINDVerifier::GetViolatingClusters)
            .def("get_supporting_baskets", &CINDVerifier::GetSupportingBaskets)
            .def("get_included_supporting_baskets", &CINDVerifier::GetIncludedSupportingBaskets)
            .def("get_included_baskets_total", &CINDVerifier::GetIncludedBasketsTotal);

    main_module.attr("cind_verification") = cind_verification_module;
}

}  // namespace python_bindings
