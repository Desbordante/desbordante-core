#include "python_bindings/ind/bind_ind_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/ind/verification_algorithms.h"
#include "core/algorithms/ind/ind.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/bind_main_classes.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindIndVerification(pybind11::module_& main_module) {
    using namespace algos;

    auto ind_verification_module = main_module.def_submodule("ind_verification");

    BindPrimitiveNoBase<INDVerifier>(ind_verification_module, "INDVerifier")
            .def("validate_ind", [](INDVerifier& verifier, model::IND& ind){
                py::dict kwargs;
                kwargs["lhs_indices"] = py::cast(ind.GetLhs().GetColumnIndices());
                kwargs["rhs_indices"] = py::cast(ind.GetRhs().GetColumnIndices());
                configure_algorithm_bind_main_classes::ConfigureAlgo(verifier, kwargs);
                verifier.Execute();
            })
            .def("ind_holds", &INDVerifier::Holds)
            .def("get_error", &INDVerifier::GetError)
            .def("get_violating_rows_count", &INDVerifier::GetViolatingRowsCount)
            .def("get_violating_clusters", &INDVerifier::GetViolatingClusters)
            .def("get_violating_clusters_count", &INDVerifier::GetViolatingClustersCount);

    main_module.attr("aind_verification") = ind_verification_module;
}
}  // namespace python_bindings
