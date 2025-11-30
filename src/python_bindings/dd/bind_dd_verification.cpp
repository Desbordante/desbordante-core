#include "python_bindings/dd/bind_dd_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/dd_verifier/dd_verifier.h"
#include "core/algorithms/dd/dd_verifier/highlight.h"
#include "core/util/create_dd.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {
namespace py = pybind11;

void BindDDVerification(py::module_& main_module) {
    using namespace algos;
    auto dd_verification_module = main_module.def_submodule("dd_verification");
    py::class_<model::DFStringConstraint>(dd_verification_module, "DF")
            .def(py::init(&util::dd::CreateDf));

    py::class_<Highlight>(dd_verification_module, "Highlight")
            .def_property_readonly("attribute_index", &Highlight::GetAttributeIndex)
            .def_property_readonly("distance", &Highlight::GetDistance)
            .def_property_readonly("pair_rows", &Highlight::GetPairRows);

    BindPrimitiveNoBase<dd::DDVerifier>(dd_verification_module, "DDVerifier")
            .def("dd_holds", &dd::DDVerifier::DDHolds)
            .def("get_error", &dd::DDVerifier::GetError)
            .def("get_num_error_pairs", &dd::DDVerifier::GetNumErrorRhs)
            .def("get_highlights", &dd::DDVerifier::GetHighlights);
}
}  // namespace python_bindings
