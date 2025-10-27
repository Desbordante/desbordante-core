#include "pac/bind_pac.h"

#include <memory>

#include <pybind11/detail/common.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "algorithms/pac/domain_pac.h"
#include "algorithms/pac/model/default_domains/ball.h"
#include "algorithms/pac/model/default_domains/parallelepiped.h"
#include "algorithms/pac/model/default_domains/untyped_domain.h"
#include "algorithms/pac/model/idomain.h"
#include "algorithms/pac/pac.h"

namespace py = pybind11;

namespace {
/// @brief Convert Domain PAC to Python tuple, which first element is string representation of
/// Domain, other elements are column names.
py::tuple DomainPACToTuple(model::DomainPAC const& d_pac) {
    auto const column_names = d_pac.GetColumnNames();
    py::tuple result(column_names.size() + 1);
    // It's unlikely that set of Domain PACs will contain PACs on different domains, so using
    // Domain's string representation shouldn't lead to a lot of collisions.
    result[0] = d_pac.GetDomain().ToString();
    // Cannot use std::copy, because py::tuple provides only const iterators
    for (std::size_t i = 0; i < column_names.size(); ++i) {
        result[i] = column_names[i];
    }
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
            .def_property_readonly("epsilon", &PAC::GetEpsilon)
            .def_property_readonly("delta", &PAC::GetDelta)
            .def("to_short_string", &PAC::ToShortString)
            .def("to_long_string", &PAC::ToLongString)
            .def("__str__", &PAC::ToLongString);
    // None of current PAC types can be pickled, because all of them contain user-defined metrics
    py::class_<DomainPAC, PAC>(pac_module, "DomainPAC")
            .def_property_readonly("domain", &DomainPAC::GetDomain,
                                   pybind11::return_value_policy::reference)
            .def_property_readonly("column_indices",
                                   [](model::DomainPAC const& d_pac) {
                                       return d_pac.GetColumns().GetColumnIndicesAsVector();
                                   })
            .def_property_readonly("column_names", &model::DomainPAC::GetColumnNames)
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
            "Uses \"radius comparer\", i. e. x < y <=> dist(x, center) < dist(y, center) and "
            "Euclidean\n"
            "distance: dist(x, y) = sqrt((d_0(x[0], y[0]))^2 + ... + (d_i(x[n], y[n]))^2), where "
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
            "D = [lower[0], upper[0]] x ... x [lower[n], upper[n]].\n"
            "Uses product comparer, i. e.x < y <=> x[0] < y[0] & ... & x[n] < y[n]\n"
            "and Chebyshev distance: dist(x, y) = max{d_0(x[0], y[0]), ..., d_n(x[n], y[n])}, "
            "where d_i is a default\n"
            "metric for i-th column's data type multiplied by leveling_coefficients[i] (which "
            "defaults to 1).\n";
    py::class_<UntypedDomain, IDomain, std::shared_ptr<UntypedDomain>>(domains_module,
                                                                       "CustomDomain")
            .def(py::init<std::function<bool(std::vector<std::string> const&,
                                             std::vector<std::string> const&)>,
                          std::function<double(std::vector<std::string> const&)>, std::string>(),
                 "comparer"_a, "dist_from_domain"_a, "name"_a = "Custom domain")
            .doc() =
            "Custom domain, defined by two functions:\n"
            "\tcomparer: must return true when first tuple is strictly less than second. Tuples "
            "are passed as lists of strings.\n"
            "\tdist_from_domain: must return distance from domain to a value, passed as list of "
            "strings.\n"
            "Third (optional) argument is domain name, which is used to make Domain PAC's string "
            "representation.\n";
}
}  // namespace python_bindings
