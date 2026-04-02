#include "bind_cind_verification.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "algorithms/cind/cind_verifier/cind_verifier.h"
#include "py_util/bind_primitive.h"

namespace py = pybind11;

namespace python_bindings {

void BindCindVerification(py::module_& main_module) {
    using algos::cind::CINDVerifier;

    py::module_ m = main_module.def_submodule("cind_verification");

    using ViolatingCluster = CINDVerifier::ViolatingCluster;

    py::class_<ViolatingCluster>(m, "ViolatingCluster")
            .def_readonly("basket_rows", &ViolatingCluster::basket_rows)
            .def_readonly("violating_rows", &ViolatingCluster::violating_rows)
            .def(py::pickle(
                    // __getstate__
                    [](ViolatingCluster const& vc) {
                        return py::make_tuple(vc.basket_rows, vc.violating_rows);
                    },
                    // __setstate__
                    [](py::tuple st) {
                        if (st.size() != 2) {
                            throw std::runtime_error("Invalid state for ViolatingCluster pickle!");
                        }
                        return ViolatingCluster{
                                .basket_rows = st[0].cast<CINDVerifier::Cluster>(),
                                .violating_rows = st[1].cast<CINDVerifier::Cluster>()};
                    }));

    BindPrimitiveNoBase<CINDVerifier>(m, "CINDVerifier")
            .def("holds", &CINDVerifier::Holds)
            .def("get_real_validity", &CINDVerifier::GetRealValidity)
            .def("get_real_completeness", &CINDVerifier::GetRealCompleteness)
            .def("get_violating_rows_count", &CINDVerifier::GetViolatingRowsCount)
            .def("get_violating_clusters_count", &CINDVerifier::GetViolatingClustersCount)
            .def("get_violating_clusters", &CINDVerifier::GetViolatingClusters)
            .def("get_supporting_baskets", &CINDVerifier::GetSupportingBaskets)
            .def("get_included_supporting_baskets", &CINDVerifier::GetIncludedSupportingBaskets)
            .def("get_included_baskets_total", &CINDVerifier::GetIncludedBasketsTotal);
}

}  // namespace python_bindings
