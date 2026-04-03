#include "python_bindings/fd/bind_fd_verification.h"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/fd/fd_input.h"
#include "core/algorithms/fd/fd_verifier/fd_verifier.h"
#include "core/algorithms/fd/fd_verifier/highlight.h"
#include "core/algorithms/fd/verification_algorithms.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;

#define FD_INPUT_CLASS_NAME "FdInput"

std::string FdInputRepr(model::FdInput const& fd_input) {
    std::stringstream ss;
    ss << FD_INPUT_CLASS_NAME << '(';
    ss << py::repr(py::cast(fd_input.lhs));
    ss << ", ";
    ss << py::repr(py::cast(fd_input.rhs));
    ss << ')';
    return ss.str();
}

std::string FdInputStr(model::FdInput const& fd_input) {
    std::stringstream ss;
    auto to_stream = [&ss](auto&& arg) { ss << arg; };
    ss << "fd specification with ";
    ss << "LHS = ";
    ss << '[';
    for (auto&& el : fd_input.lhs) {
        std::visit(to_stream, el);
        ss << ' ';
    }
    ss.seekp(-1, std::stringstream::cur);
    ss << ']';
    ss << ", RHS = ";
    ss << '[';
    for (auto&& el : fd_input.rhs) {
        std::visit(to_stream, el);
        ss << ' ';
    }
    ss.seekp(-1, std::stringstream::cur);
    ss << ']';
    return ss.str();
}
}  // namespace

namespace python_bindings {
void BindFdVerification(pybind11::module_& main_module) {
    using namespace algos;
    using namespace algos::fd_verifier;
    using namespace pybind11::literals;
    using model::FdInput;

    auto fd_verification_module = main_module.def_submodule("fd_verification");
    py::class_<FdInput>(fd_verification_module, FD_INPUT_CLASS_NAME)
            .def_readwrite("lhs", &FdInput::lhs)
            .def_readwrite("rhs", &FdInput::rhs)
            .def("__repr__", FdInputRepr)
            .def("__str__", FdInputStr)
            .def(py::self == py::self)
            .def(py::self != py::self)
            .def(py::init<std::vector<std::variant<std::string, model::Index>>,
                          std::vector<std::variant<std::string, model::Index>>>(),
                 "lhs"_a, "rhs"_a);
    py::class_<Highlight>(fd_verification_module, "Highlight")
            .def_property_readonly("cluster", &Highlight::GetCluster)
            .def_property_readonly("num_distinct_rhs_values", &Highlight::GetNumDistinctRhsValues)
            .def_property_readonly("most_frequent_rhs_value_proportion",
                                   &Highlight::GetMostFrequentRhsValueProportion);
    BindPrimitiveNoBase<FDVerifier>(fd_verification_module, "FDVerifier")
            .def("fd_holds", &FDVerifier::FDHolds)
            .def("get_error", &FDVerifier::GetError)
            .def("get_num_error_clusters", &FDVerifier::GetNumErrorClusters)
            .def("get_num_error_rows", &FDVerifier::GetNumErrorRows)
            .def("get_highlights", &FDVerifier::GetHighlights);

    main_module.attr("afd_verification") = fd_verification_module;
}
}  // namespace python_bindings
