#include "bind_ind_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/ind/verification_algorithms.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindIndVerification(pybind11::module_& main_module) {
    using namespace algos;

    auto ind_verification_module = main_module.def_submodule("ind_verification");

    BindPrimitiveNoBase<INDVerifier>(ind_verification_module, "INDVerifier")
            .def("ind_holds", &INDVerifier::Holds)
            .def("get_error", &INDVerifier::GetError)
            .def("get_violating_rows_count", &INDVerifier::GetViolatingRowsCount)
            .def("get_violating_clusters", &INDVerifier::GetViolatingClusters)
            .def("get_violating_clusters_count", &INDVerifier::GetViolatingClustersCount);

    main_module.attr("aind_verification") = ind_verification_module;
}
}  // namespace python_bindings
