#include "python_bindings/pac/bind_pac_verification.h"

#include <pybind11/pybind11.h>

#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/fd_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/fd_pac_verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace py = pybind11;
using namespace algos::pac_verifier;

namespace python_bindings {
void BindPACVerification(py::module_& main_module) {
    using namespace model;
    using namespace std::string_literals;
    using namespace pybind11::literals;

    auto pac_verification_module = main_module.def_submodule("pac_verification");
    auto algos_module = pac_verification_module.def_submodule("algorithms");

    BindDomainPACVerification(pac_verification_module);
    BindFDPACVerification(pac_verification_module, algos_module);
}

void BindDomainPACVerification(py::module_& pac_verification_module) {
    using namespace algos::pac_verifier;
    using namespace pybind11::literals;
    using namespace std::string_literals;

    py::class_<DomainPACHighlight>(pac_verification_module, "DomainPACHighlight")
            .def_property_readonly("indices", &DomainPACHighlight::GetRowNums)
            .def_property_readonly("string_data", &DomainPACHighlight::GetStringData)
            .def("__str__", [](DomainPACHighlight const& hl) -> std::string {
                auto strings = hl.GetStringData();
                if (strings.empty()) {
                    return "";
                }
                return "["s +
                       std::accumulate(std::next(strings.begin()), strings.end(), strings.front(),
                                       [](std::string&& a, std::string const& b) {
                                           return std::move(a) + ", " + b;
                                       }) +
                       "]"s;
            });

    auto domain_pac_verifier =
            BindPrimitiveNoBase<DomainPACVerifier>(pac_verification_module, "DomainPACVerifier")
                    .def("get_pac", &DomainPACVerifier::GetPAC)
                    .def("get_highlights", &DomainPACVerifier::GetHighlights, "eps_1"_a = -1,
                         "eps_2"_a = -1);
}

void BindFDPACVerification(py::module_& pac_verification_module, py::module_& algorithms_module) {
    using namespace py::literals;

    py::class_<FDPACHighlight>(pac_verification_module, "FDPACHighlight")
            .def_property_readonly("row_indices", &FDPACHighlight::RowIndices)
            .def_property_readonly("num_pairs", &FDPACHighlight::NumPairs)
            .def_property_readonly("string_data", &FDPACHighlight::StringData)
            .def("__str__", &FDPACHighlight::ToString)
            .doc() = "A set of tuple pairs that violate FD PAC.";

    // Have to use low-level facility, because BindPrimitiveNoBase would overwrite ".default"
    // attribute on each call
    detail::RegisterAlgorithm<FDPACVerifier, algos::Algorithm>(algorithms_module, "FDPACVerifier")
            .def("get_pac", &FDPACVerifier::GetPAC)
            .def("get_highlights", &FDPACVerifier::GetHighlights, "eps_1"_a = 0, "eps_2"_a = -1);
}
}  // namespace python_bindings
