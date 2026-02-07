#include "python_bindings/pac/bind_pac.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <pybind11/detail/common.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "core/algorithms/pac/domain_pac.h"
#include "core/algorithms/pac/model/default_domains/ball.h"
#include "core/algorithms/pac/model/default_domains/parallelepiped.h"
#include "core/algorithms/pac/model/default_domains/untyped_domain.h"
#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/pac.h"

namespace py = pybind11;

namespace {
/// @brief Convert Domain PAC to Python tuple, which first element is string representation of
/// Domain, then follow column names, and then epsilon and delta.
py::tuple DomainPACToTuple(model::DomainPAC const& d_pac) {
    auto const column_names = d_pac.GetColumnNames();
    py::tuple result(column_names.size() + 3);
    // It's unlikely that set of Domain PACs will contain PACs on different domains, so using
    // Domain's string representation shouldn't lead to a lot of collisions.
    result[0] = d_pac.GetDomain().ToString();
    // Cannot use std::copy, because py::tuple provides only const iterators
    for (std::size_t i = 0; i < column_names.size(); ++i) {
        result[i] = column_names[i];
    }
    result[column_names.size() + 1] = d_pac.GetEpsilon();
    result[column_names.size() + 2] = d_pac.GetDelta();
    return result;
}
}  // namespace

namespace python_bindings {
void BindPAC(py::module& main_module) {
    using namespace model;
    using namespace pac::model;
    using namespace pybind11::literals;

    auto pac_module = main_module.def_submodule("pac");

    // PACs
    py::class_<PAC>(pac_module, "PAC")
            .def("to_short_string", &PAC::ToShortString)
            .def("to_long_string", &PAC::ToLongString)
            .def("__str__", &PAC::ToLongString);
    // None of current PAC types can be pickled, because all of them contain user-defined metrics
    py::class_<DomainPAC, PAC>(pac_module, "DomainPAC")
            .def_property_readonly("epsilon", &PAC::GetEpsilon)
            .def_property_readonly("delta", &PAC::GetDelta)
            .def_property_readonly("domain", &DomainPAC::GetDomain,
                                   pybind11::return_value_policy::reference)
            .def_property_readonly("column_indices",
                                   [](model::DomainPAC const& d_pac) {
                                       return d_pac.GetColumns().GetColumnIndicesAsVector();
                                   })
            .def_property_readonly("column_names", &model::DomainPAC::GetColumnNames)
            .def("__eq__",
                 [](DomainPAC const& a, DomainPAC const& b) {
                     return a.GetDomain().ToString() == b.GetDomain().ToString() &&
                            a.GetColumns() == b.GetColumns() && a.GetEpsilon() == b.GetEpsilon() &&
                            a.GetDelta() == b.GetDelta();
                 })
            .def("__hash__",
                 [](DomainPAC const& d_pac) { return py::hash(DomainPACToTuple(d_pac)); });

    // Domains
    auto domains_module = pac_module.def_submodule("domains");
    py::class_<IDomain, std::shared_ptr<IDomain>>(domains_module, "IDomain")
            .def("__str__", &IDomain::ToString)
            .doc() =
            "Abstract base class for domains. Cannot be used directly.\n"
            "Currently available domain types: Ball, Parallelepiped, CustomDomain.\n";
    py::class_<Ball, IDomain, std::shared_ptr<Ball>>(domains_module, "Ball")
            .def(py::init<std::vector<std::string>, double, std::vector<double>>(), "center"_a,
                 "radius"_a, "leveling_coefficients"_a = std::vector<double>{})
            .doc() =
            "A closed n-ary ball, defined by center and radius, i. e. D = {x : dist(center, x) <= "
            "radius}.\n"
            "Uses Euclidean distance:\n"
            "dist(x, y) = sqrt((d_1(x[1], y[1]))^2 + ... + (d_n(x[n], y[n]))^2), where "
            "d_i is a default\n"
            "metric for i-th column's data type multiplied by leveling_coefficients[i] (which "
            "defaults to 1).\n";
    py::class_<Parallelepiped, IDomain, std::shared_ptr<Parallelepiped>>(domains_module,
                                                                         "Parallelepiped")
            .def(py::init<std::vector<std::string>, std::vector<std::string>,
                          std::vector<double>>(),
                 "lower_bound"_a, "upper_bound"_a,
                 "leveling_coefficients"_a = std::vector<double>{})
            .def(py::init<std::string, std::string>(), "lower_bound"_a, "upper_bound"_a)
            .doc() =
            "A closed n-ary parallelepiped, defined by lower and upper bounds, i. e.\n"
            "D = [lower[1], upper[1]] x ... x [lower[n], upper[n]].\n"
            "Uses Chebyshev distance: dist(x, y) = max{d_1(x[1], y[1]), ..., d_n(x[n], y[n])}, "
            "where d_i is a default\n"
            "metric for i-th column's data type multiplied by leveling_coefficients[i] (which "
            "defaults to 1).\n";
    py::class_<UntypedDomain, IDomain, std::shared_ptr<UntypedDomain>>(domains_module,
                                                                       "CustomDomain")
            .def(py::init<std::function<double(std::vector<std::string> const&)>, std::string>(),
                 "dist_from_domain"_a, "name"_a = "Custom domain")
            .doc() =
            "Custom domain, defined by metric:\n"
            "\tdist_from_domain: must return distance from domain to a value, passed as list of "
            "strings.\n"
            "Second (optional) argument is domain name, which is used to make Domain PAC's string "
            "representation.\n";
}
}  // namespace python_bindings
