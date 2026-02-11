#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/ar/ar_verifier/ar_verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindARVerification(py::module_& main_module) {
    using namespace algos;
    using namespace algos::ar_verifier;

    auto ar_verification_module = main_module.def_submodule("ar_verification");

    BindPrimitiveNoBase<ARVerifier>(ar_verification_module, "ARVerifier")
            .def("ar_holds", &ARVerifier::ARHolds)
            .def("get_num_clusters_violating_ar", &ARVerifier::GetNumClustersViolatingAR)
            .def("get_num_transactions_violating_ar", &ARVerifier::GetNumTransactionsViolatingAR)
            .def("get_num_transactions_satisfying_ar", &ARVerifier::GetNumTransactionsSatisfyingAR)
            .def("get_clusters_violating_ar", &ARVerifier::GetClustersViolatingAR)
            .def("get_real_support", &ARVerifier::GetRealSupport)
            .def("get_real_confidence", &ARVerifier::GetRealConfidence);

    main_module.attr("ar_verification") = ar_verification_module;
}
}  // namespace python_bindings
