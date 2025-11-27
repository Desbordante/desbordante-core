#include "python_bindings/od/bind_od_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/od/set_based_verifier/verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindAODVerification(pybind11::module_& main_module) {
    using namespace algos;

    auto od_verification_module = main_module.def_submodule("aod_verification");

    auto set_based_aod_verifier =
            BindPrimitiveNoBase<algos::od::SetBasedAodVerifier>(od_verification_module,
                                                                "SetBasedAodVerifier")
                    .def("holds", &algos::od::SetBasedAodVerifier::Holds, py::arg("error") = 0.0)
                    .def("get_error", &algos::od::SetBasedAodVerifier::GetError)
                    .def("get_removal_set", &algos::od::SetBasedAodVerifier::GetRemovalSet);

    main_module.attr("aod_verification") = od_verification_module;
}
}  // namespace python_bindings
