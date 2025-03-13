#include "bind_dd_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "util/create_dd.h"
#include "algorithms/dd/dd_verifier/dd_verifier.h"
#include "algorithms/dd/dd.h"
#include "py_util/bind_primitive.h"

namespace python_bindings {
namespace py = pybind11;
    void BindDDVerification(py::module_& main_module) {
        using namespace algos;
        auto dd_verification_module = main_module.def_submodule("dd_verification");
        dd_verification_module.def("create_df", &util::dd::СreateDf);
        dd_verification_module.def("create_dd", &util::dd::СreateDd);
        py::class_<model::DFStringConstraint>(dd_verification_module, "DF");


        BindPrimitiveNoBase<dd::DDVerifier>(dd_verification_module, "DDVerifier")
                .def("dd_holds", &dd::DDVerifier::DDHolds)
                .def("get_error", &dd::DDVerifier::GetError)
                .def("get_num_error_pairs", &dd::DDVerifier::GetNumErrorRhs)
                .def("get_highlights", &dd::DDVerifier::GetHighlights);
    }
}  // namespace python_bindings