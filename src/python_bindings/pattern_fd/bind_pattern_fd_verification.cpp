#include "bind_pattern_fd_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/fd/pattern_fd_verifier/pattern_fd_verifier.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindPatternFDVerification(py::module_ &main_module) {
    using namespace algos;
    using namespace algos::pattern_fd;

    auto pattern_fd_verification_module = main_module.def_submodule("pattern_fd_verification");

    py::class_<PatternInfo, std::shared_ptr<PatternInfo>>(pattern_fd_verification_module,
                                                          "PatternInfo")
            .def("type", &PatternInfo::Type);

    py::class_<TokenPatternInfo, PatternInfo, std::shared_ptr<TokenPatternInfo>>(
            pattern_fd_verification_module, "Token")
            .def(py::init<std::string const &, Pos>(), py::arg("token"), py::arg("position"))
            .def("type", &TokenPatternInfo::Type)
            .def_property_readonly(
                    "token", [](TokenPatternInfo const &self) { return std::string(self.Token()); })
            .def_property_readonly("position", &TokenPatternInfo::Position);

    py::class_<RegexPatternInfo, PatternInfo, std::shared_ptr<RegexPatternInfo>>(
            pattern_fd_verification_module, "Regex")
            .def(py::init<std::string const &>(), py::arg("pattern"))
            .def("type", &RegexPatternInfo::Type);

    py::class_<WildcardPatternInfo, PatternInfo, std::shared_ptr<WildcardPatternInfo>>(
            pattern_fd_verification_module, "Wildcard")
            .def(py::init<>())
            .def("type", &WildcardPatternInfo::Type);

    py::class_<Highlight>(pattern_fd_verification_module, "Highlight")
            .def_property_readonly("cluster", &Highlight::GetCluster)
            .def_property_readonly("get_violating_rows", &Highlight::GetViolatingRows);

    BindPrimitiveNoBase<PatternFDVerifier>(pattern_fd_verification_module, "PatternFDVerifier")
            .def("get_possible_options", &PatternFDVerifier::GetPossibleOptions)
            .def("pattern_fd_holds", &PatternFDVerifier::PatternFDHolds)
            .def("get_real_pattern_fd_coverage", &PatternFDVerifier::GetRealPatternFDCoverage)
            .def("get_real_min_pattern_inclusion", &PatternFDVerifier::GetRealMinPatternInclusion)
            .def("get_real_max_rhs_deviation", &PatternFDVerifier::GetRealMaxRhsDeviation)
            .def("get_num_error_rows", &PatternFDVerifier::GetNumErrorRows)
            .def("get_clusters", &PatternFDVerifier::GetClusters)
            .def("get_num_error_clusters", &PatternFDVerifier::GetNumErrorClusters);
    main_module.attr("pattern_fd_verification") = pattern_fd_verification_module;
}
}  // namespace python_bindings
