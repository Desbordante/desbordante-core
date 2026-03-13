#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/sd/sd_verifier/sd_verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}

namespace python_bindings {
void BindSDVerification(py::module_& main_module) {
    using namespace algos;
    using namespace algos::sd_verifier;

    auto sd_verification_module = main_module.def_submodule("sd_verification");

    py::class_<SDDeletion>(sd_verification_module, "SDDeletion")
            .def_readonly("row_idx", &SDDeletion::row_idx);

    py::class_<SDInsertion>(sd_verification_module, "SDInsertion")
            .def_readonly("left_row_idx", &SDInsertion::left_row_idx)
            .def_readonly("right_row_idx", &SDInsertion::right_row_idx)
            .def_readonly("val_left", &SDInsertion::val_left)
            .def_readonly("val_right", &SDInsertion::val_right)
            .def_readonly("min_insertions", &SDInsertion::min_insertions)
            .def_readonly("max_insertions", &SDInsertion::max_insertions);

    BindPrimitiveNoBase<SDVerifier>(sd_verification_module, "SDVerifier")
            .def("get_ops", &SDVerifier::GetOPS)
            .def("get_violations", &SDVerifier::GetViolations)
            .def("get_error", &SDVerifier::GetError)
            .def("get_confidence", &SDVerifier::GetConfidence)
            .def("holds", &SDVerifier::Holds);

    main_module.attr("sd_verification") = sd_verification_module;
}
}  // namespace python_bindings
