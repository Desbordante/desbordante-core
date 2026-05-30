#include "python_bindings/pac/bind_pac.h"

#include <pybind11/pybind11.h>

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "core/algorithms/pac/domain_pac.h"
#include "core/config/column_index/type.h"
#include "python_bindings/pac/bind_domains.h"

namespace py = pybind11;

namespace {
/// @brief Convert Domain PAC to Python tuple, which first element is string representation of
/// Domain, then follow column indices.
/// Epsilon and delta are not included in tuple, because there's no proper way to hash doubles
py::tuple DomainPACToTuple(model::DomainPAC const& d_pac) {
    auto const column_indices = d_pac.GetColumnIndices();
    py::tuple result(column_indices.size() + 1);
    // It's unlikely that set of Domain PACs will contain PACs on different domains, so using
    // Domain's string representation shouldn't lead to a lot of collisions.
    result[0] = d_pac.GetDomainName();
    // Cannot use std::copy, because py::tuple provides only const iterators
    for (std::size_t i = 0; i < column_indices.size(); ++i) {
        result[i + 1] = column_indices[i];
    }
    return result;
}
}  // namespace

namespace python_bindings {
void BindPAC(py::module& main_module) {
    using namespace model;
    using namespace pybind11::literals;

    auto pac_module = main_module.def_submodule("pac");

    // Domain PACs cannot be pickled, because they contain user-defined metrics
    py::class_<DomainPAC>(pac_module, "DomainPAC")
            .def_property_readonly("epsilon", &DomainPAC::GetEpsilon)
            .def_property_readonly("delta", &DomainPAC::GetDelta)
            .def_property_readonly("domain", &DomainPAC::GetDomainName)
            .def_property_readonly("column_indices", &model::DomainPAC::GetColumnIndices)
            .def_property_readonly("column_names", &model::DomainPAC::GetColumnNames)
            .def("to_short_string", &DomainPAC::ToShortString)
            .def("to_long_string", &DomainPAC::ToLongString)
            .def("__str__", &DomainPAC::ToLongString)
            .def(py::self == py::self)
            .def("__hash__",
                 [](DomainPAC const& d_pac) { return py::hash(DomainPACToTuple(d_pac)); })
            .def(py::pickle(
                    [](DomainPAC const& d_pac) {
                        return py::make_tuple(d_pac.GetColumnIndices(), d_pac.GetColumnNames(),
                                              d_pac.GetDomainName(), d_pac.GetEpsilon(),
                                              d_pac.GetDelta());
                    },
                    [](py::tuple t) {
                        if (t.size() != 5) {
                            throw std::runtime_error("Invalid state for Domain PAC pickle");
                        }
                        auto column_indices = t[0].cast<std::vector<config::IndexType>>();
                        auto column_names = t[1].cast<std::vector<std::string>>();
                        auto domain_name = t[2].cast<std::string>();
                        auto epsilon = t[3].cast<double>();
                        auto delta = t[4].cast<double>();
                        return DomainPAC(epsilon, delta, domain_name, std::move(column_indices),
                                         std::move(column_names));
                    }));

    BindDomains(pac_module);
}
}  // namespace python_bindings
