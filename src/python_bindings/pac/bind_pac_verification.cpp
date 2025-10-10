#include "python_bindings/pac/bind_pac_verification.h"

#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/pac/domain_pac.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier_cli_adapter.h"
#include "core/algorithms/pac/pac_verifier/pac_verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace py = pybind11;

namespace python_bindings {
/// @brief Register concrete PAC verifier.
/// Inheritance cannot be used for this purpose, because abstract PAC cannot be copied (and PAC
/// *must* be copied out from algorithm, becuase algortihm can be executed again and overwrite PAC)
template <typename VerifierT, typename PACType>
auto BindPACVerifier(py::module_& algos_module, auto&& name) {
    // BindPrimitiveNoBase cannot be used here, becuase it cannot bind different methods for
    // different classes.
    return detail::RegisterAlgorithm<VerifierT, algos::pac_verifier::PACVerifier>(algos_module,
                                                                                  std::move(name))
            .def(
                    "get_pac",
                    [](VerifierT const& ver) { return dynamic_cast<PACType const&>(ver.GetPAC()); },
                    py::return_value_policy::copy);
}

void BindPACVerification(py::module_& main_module) {
    using namespace algos::pac_verifier;
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
}

void BindDomainPACVerification(py::module_& pac_verification_module, py::module_& algos_module,
                               py::module_& cli_module) {
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
}  // namespace python_bindings
