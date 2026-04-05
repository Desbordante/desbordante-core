#include "python_bindings/pac/bind_pac.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <pybind11/detail/common.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "core/algorithms/pac/domain_pac.h"
#include "core/algorithms/pac/fd_pac.h"
#include "core/algorithms/pac/model/default_domains/ball.h"
#include "core/algorithms/pac/model/default_domains/parallelepiped.h"
#include "core/algorithms/pac/model/default_domains/untyped_domain.h"
#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/pac.h"
#include "core/model/table/column.h"
#include "core/model/table/vertical.h"
#include "python_bindings/py_util/table_serialization.h"

namespace py = pybind11;

namespace {
/// @brief Convert Domain PAC to Python tuple, which first element is string representation of
/// Domain, then follow column indices.
/// Epsilon and delta are not included in tuple, because there's no proper way to hash doubles
py::tuple DomainPACToTuple(model::DomainPAC const& d_pac) {
    auto const column_indices = d_pac.GetColumns().GetColumnIndicesAsVector();
    py::tuple result(column_indices.size() + 1);
    // It's unlikely that set of Domain PACs will contain PACs on different domains, so using
    // Domain's string representation shouldn't lead to a lot of collisions.
    result[0] = d_pac.GetDomain().ToString();
    // Cannot use std::copy, because py::tuple provides only const iterators
    for (std::size_t i = 0; i < column_indices.size(); ++i) {
        result[i] = column_indices[i];
    }
    return result;
}

/// @brief Convert FD PAC to a Python tuple, which elements are (concatenated):
///   1. Lhs column indices
///	  2. Rhs column indices
/// Lhs deltas, epsilons and delta are not included in tuple, because there's no proper way to hash
/// doubles
py::tuple FDPAcToTuple(model::FDPAC const& fd_pac) {
    auto const lhs_col_indices = fd_pac.GetLhs().GetColumnIndicesAsVector();
    auto const rhs_col_indices = fd_pac.GetRhs().GetColumnIndicesAsVector();

    auto add_to_tuple = [](py::tuple& tp, auto const& vec, std::size_t& shift) {
        for (std::size_t i = 0; i < vec.size(); ++i) {
            tp[shift + i] = vec[i];
        }
        shift += vec.size();
    };

    py::tuple result(lhs_col_indices.size() + rhs_col_indices.size());
    std::size_t shift = 0;
    add_to_tuple(result, lhs_col_indices, shift);
    add_to_tuple(result, rhs_col_indices, shift);

    return result;
}
}  // namespace

namespace python_bindings {
void BindPAC(py::module& main_module) {
    using namespace model;
    using namespace pac::model;
    using namespace pybind11::literals;
    using namespace table_serialization;

    constexpr static double kEpsilon = 1e-12;
    auto approx_equal = [](double a, double b) { return std::abs(a - b) < kEpsilon; };

    auto pac_module = main_module.def_submodule("pac");

    // PACs
    py::class_<PAC>(pac_module, "PAC")
            .def("to_short_string", &PAC::ToShortString)
            .def("to_long_string", &PAC::ToLongString)
            .def("__str__", &PAC::ToLongString);

    // Domain PACs cannot be pickled, because they contain user-defined metrics
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
                 [approx_equal](DomainPAC const& a, DomainPAC const& b) {
                     return a.GetDomain().ToString() == b.GetDomain().ToString() &&
                            a.GetColumns() == b.GetColumns() &&
                            approx_equal(a.GetEpsilon(), b.GetEpsilon()) &&
                            approx_equal(a.GetDelta(), b.GetDelta());
                 })
            .def("__hash__",
                 [](DomainPAC const& d_pac) { return py::hash(DomainPACToTuple(d_pac)); });

    auto get_col_names = [](Vertical const& vert) {
        std::vector<std::string> col_names;
        col_names.reserve(vert.GetArity());
        std::ranges::transform(vert.GetColumns(), std::back_inserter(col_names),
                               [](Column const* col) { return col->GetName(); });
        return col_names;
    };
    py::class_<FDPAC, PAC>(pac_module, "FDPAC")
            .def_property_readonly("epsilons", &PAC::GetEpsilons)
            .def_property_readonly("deltas", &PAC::GetDeltas)
            .def_property_readonly("lhs_deltas", &FDPAC::GetLhsDeltas)
            .def_property_readonly(
                    "lhs_indices",
                    [](FDPAC const& fd_pac) { return fd_pac.GetLhs().GetColumnIndicesAsVector(); })
            .def_property_readonly(
                    "rhs_indices",
                    [](FDPAC const& fd_pac) { return fd_pac.GetRhs().GetColumnIndicesAsVector(); })
            .def_property_readonly("lhs_column_names",
                                   [&get_col_names](FDPAC const& fd_pac) {
                                       return get_col_names(fd_pac.GetLhs());
                                   })
            .def_property_readonly("rhs_column_names",
                                   [&get_col_names](FDPAC const& fd_pac) {
                                       return get_col_names(fd_pac.GetRhs());
                                   })
            .def("__eq__",
                 [approx_equal](FDPAC const& a, FDPAC const& b) {
                     return a.GetLhs() == b.GetLhs() && a.GetRhs() == b.GetRhs() &&
                            std::ranges::equal(a.GetLhsDeltas(), b.GetLhsDeltas(), approx_equal) &&
                            std::ranges::equal(a.GetEpsilons(), b.GetEpsilons(), approx_equal) &&
                            approx_equal(a.GetDelta(), b.GetDelta());
                 })
            .def("__hash__", [](FDPAC const& fd_pac) { return py::hash(FDPAcToTuple(fd_pac)); })
            .def(py::pickle(
                    // __getstate__
                    [](FDPAC const& fd_pac) {
                        auto schema_state = SerializeRelationalSchema(fd_pac.GetRelSchema().get());
                        auto lhs_state = SerializeVertical(fd_pac.GetLhs());
                        auto rhs_state = SerializeVertical(fd_pac.GetRhs());
                        return py::make_tuple(std::move(schema_state), std::move(lhs_state),
                                              std::move(rhs_state), fd_pac.GetLhsDeltas(),
                                              fd_pac.GetEpsilons(), fd_pac.GetDelta());
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 6) {
                            throw std::runtime_error("Invalid state for FD PAC pickle");
                        }
                        auto schema = DeserializeRelationalSchema(t[0].cast<py::tuple>());
                        auto lhs = DeserializeVertical(t[1].cast<py::tuple>(), schema.get());
                        auto rhs = DeserializeVertical(t[2].cast<py::tuple>(), schema.get());
                        auto lhs_deltas = t[3].cast<std::vector<double>>();
                        auto epsilons = t[4].cast<std::vector<double>>();
                        auto delta = t[5].cast<double>();
                        return FDPAC(std::move(schema), std::move(lhs), std::move(rhs),
                                     std::move(lhs_deltas), std::move(epsilons), delta);
                    }));

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
