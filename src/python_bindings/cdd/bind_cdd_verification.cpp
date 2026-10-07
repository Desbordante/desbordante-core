#include "python_bindings/cdd/bind_cdd_verification.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/cdd/cdd_verifier/cdd_verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {
namespace py = pybind11;

void BindCDDVerification(py::module_& main_module) {
    using namespace algos::cdd;
    auto cdd_verification_module = main_module.def_submodule("cdd_verification");

    py::class_<CDDHighlight>(cdd_verification_module, "CDDHighlight")
            .def_property_readonly("row_index", &CDDHighlight::GetRowIndex)
            .def_property_readonly("condition_index", &CDDHighlight::GetConditionIndex);

    BindPrimitiveNoBase<CDDVerifier>(cdd_verification_module, "CDDVerifier")
            .def("get_cdd_highlights", &CDDVerifier::GetCddHighlights)
            .def("get_highlights", &CDDVerifier::GetHighlights)
            .def("dd_holds", &CDDVerifier::DDHolds)
            .def("get_error", &CDDVerifier::GetError)
            .def("get_num_error_pairs", &CDDVerifier::GetNumErrorRhs)
            .def("get_num_cond_violations", &CDDVerifier::GetNumCondViolations);
}
}  // namespace python_bindings
