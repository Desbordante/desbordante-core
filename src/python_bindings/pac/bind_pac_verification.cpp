#include "python_bindings/pac/bind_pac_verification.h"

#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/pac/domain_pac.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier_cli_adapter.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/column_metric.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/fd_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/fd_pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace py = pybind11;
using namespace algos::pac_verifier;

namespace python_bindings {
/// @brief Register concrete PAC verifier.
/// Inheritance cannot be used for this purpose, because abstract PAC cannot be copied (and PAC
/// *must* be copied out from algorithm, because algorithm can be executed again and overwrite PAC)
template <typename VerifierT, typename PACType>
auto BindPACVerifier(py::module_& algos_module, auto&& name) {
    // BindPrimitiveNoBase cannot be used here, because it cannot bind different methods for
    // different classes.
    return detail::RegisterAlgorithm<VerifierT, algos::pac_verifier::PACVerifier>(algos_module,
                                                                                  std::move(name))
            .def(
                    "get_pac",
                    [](VerifierT const& ver) { return dynamic_cast<PACType const&>(ver.GetPAC()); },
                    py::return_value_policy::copy);
}

void BindPACVerification(py::module_& main_module) {
    using namespace model;
    using namespace std::string_literals;
    using namespace pybind11::literals;

    auto pac_verification_module = main_module.def_submodule("pac_verification");

    auto algos_module = pac_verification_module.def_submodule("algorithms");
    py::class_<PACVerifier, algos::Algorithm>(algos_module, "PACVerifier")
            // Just define interface. It will be overwritten in derived classes.
            .def("get_pac", []() {
                throw py::attribute_error{"Cannot call get_pac on abstract class PACVerifier"};
            });
    auto cli_module = algos_module.def_submodule("cli");

    BindDomainPACVerification(pac_verification_module, algos_module, cli_module);
    BindFDPACVerification(pac_verification_module, algos_module, cli_module);
}

void BindDomainPACVerification(py::module_& pac_verification_module, py::module_& algos_module,
                               py::module_& cli_module) {
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
            BindPACVerifier<DomainPACVerifier, model::DomainPAC>(algos_module, "DomainPACVerifier")
                    .def("get_highlights", &DomainPACVerifier::GetHighlights, "eps_1"_a = -1,
                         "eps_2"_a = -1);
    algos_module.attr("Default") = domain_pac_verifier;

    auto domain_pac_verifier_cli =
            detail::RegisterAlgorithm<DomainPACVerifierCLIAdapter, PACVerifier>(
                    cli_module, "DomainPACVerifierCLI");
    domain_pac_verifier_cli.doc() =
            "NOTE: This algorithm is a wrapper around DomainPACVerifer with a restricted set of "
            "options, which should be used only in CLI.\n"
            "Consider using desbordante.pac_verification.algorithms.DomainPACVerifer in Python.\n" +
            domain_pac_verifier_cli.doc().cast<std::string>();
}

void BindFDPACVerificaion(py::module_& pac_verification_module, py::module_& algos_module,
                          py::module_& cli_module) {
    using namespace py::literals;

    py::class_<FDPACHighlight>(pac_verification_module, "FDPACHighlight")
            .def_property_readonly("row_indices", &FDPACHighlight::RowIndices)
            .def_property_readonly("num_pairs", &FDPACHighlight::NumPairs)
            .def_property_readonly("string_data", &FDPACHighlight::StringData)
            .def("__str__", &FDPACHighlight::ToString)
            .doc() = "A set of tuple pairs that violate FD PAC.";

    py::class_<algos::pac_verifier::detail::FakeValueMetric>(pac_verification_module, "ValueMetric")
            .doc() =
            "A wrapper around [None | Callable], where Callable has signature (str, str) -> "
            "float.\n"
            "Most probably, you won't need to use this class directly.";

    auto fd_pac_verifier =
            BindPACVerifier<FDPACVerifier<true>, model::FDPAC>(algos_module, "FDPACVerifier")
                    .def("get_highlights", &FDPACVerifier<true>::GetHighlights, "eps_1"_a = 0,
                         "eps_2"_a = -1);
    algos_module.attr("Default") = fd_pac_verifier;

    auto fd_pac_verifier_cli =
            BindPACVerifier<FDPACVerifier<false>, model::FDPAC>(cli_module, "FDPACVerifierCLI");
    fd_pac_verifier_cli.doc() =
            "NOTE: This algorithm is a wrapper around FDPACVerifier with a restricted set of "
            "options, which should be used only in CLI.\n"
            "Consider using desbordante.pac_verification.algorithms.FDPACVerifier in Python.\n" +
            fd_pac_verifier_cli.doc().cast<std::string>();
}
}  // namespace python_bindings
