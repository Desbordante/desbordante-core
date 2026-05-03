#include "python_bindings/pac/bind_pac.h"

#include <pybind11/pybind11.h>

#include <cstddef>
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
#include "core/algorithms/pac/fd_pac.h"
#include "core/algorithms/pac/ucc_pac.h"
#include "core/config/column_index/type.h"
#include "core/config/indices/type.h"
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

/// @brief Convert FD PAC to a Python tuple, which elements are (concatenated):
///   1. Lhs column indices
///   2. Rhs column indices
/// Lhs deltas, epsilons and delta are not included in tuple, because there's no proper way to hash
/// doubles
py::tuple FDPACToTuple(model::FDPAC const& fd_pac) {
    auto const lhs_col_indices = fd_pac.GetLhsColumnIndices();
    auto const rhs_col_indices = fd_pac.GetRhsColumnIndices();

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

/// @brief Convert UCC PAC to a Python tuple of column indices
py::tuple UCCPACToTuple(model::UCCPAC const& ucc_pac) {
    auto const column_indices = ucc_pac.GetColumnIndices();
    py::tuple result(column_indices.size());
    for (std::size_t i = 0; i < column_indices.size(); ++i) {
        result[i] = column_indices[i];
    }
    return result;
}
}  // namespace

namespace python_bindings {
void BindPAC(py::module& main_module) {
    using namespace model;
    using namespace pybind11::literals;

    auto pac_module = main_module.def_submodule("pac");

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

    py::class_<FDPAC>(pac_module, "FDPAC")
            .def_property_readonly("epsilons", &FDPAC::GetEpsilons)
            .def_property_readonly("delta", &FDPAC::GetDelta)
            .def_property_readonly("lhs_deltas", &FDPAC::GetLhsDeltas)
            .def_property_readonly("lhs_indices", &FDPAC::GetLhsColumnIndices)
            .def_property_readonly("lhs_column_names", &FDPAC::GetLhsColumnNames)
            .def_property_readonly("rhs_indices", &FDPAC::GetRhsColumnIndices)
            .def_property_readonly("rhs_column_names", &FDPAC::GetRhsColumnNames)
            .def("to_short_string", &FDPAC::ToShortString)
            .def("to_long_string", &FDPAC::ToLongString)
            .def("__str__", &FDPAC::ToLongString)
            .def(py::self == py::self)
            .def("__hash__", [](FDPAC const& fd_pac) { return py::hash(FDPACToTuple(fd_pac)); })
            .def(py::pickle(
                    [](FDPAC const& fd_pac) {
                        return py::make_tuple(
                                fd_pac.GetLhsColumnIndices(), fd_pac.GetLhsColumnNames(),
                                fd_pac.GetRhsColumnIndices(), fd_pac.GetRhsColumnNames(),
                                fd_pac.GetLhsDeltas(), fd_pac.GetEpsilons(), fd_pac.GetDelta());
                    },
                    [](py::tuple t) {
                        if (t.size() != 7) {
                            throw std::runtime_error("Invalid state for FD PAC pickle");
                        }
                        auto lhs_indices = t[0].cast<config::IndicesType>();
                        auto lhs_column_names = t[1].cast<std::vector<std::string>>();
                        auto rhs_indices = t[2].cast<config::IndicesType>();
                        auto rhs_column_names = t[3].cast<std::vector<std::string>>();
                        auto lhs_deltas = t[4].cast<std::vector<double>>();
                        auto epsilons = t[5].cast<std::vector<double>>();
                        auto delta = t[6].cast<double>();
                        return FDPAC(std::move(lhs_indices), std::move(lhs_column_names),
                                     std::move(rhs_indices), std::move(rhs_column_names),
                                     std::move(lhs_deltas), std::move(epsilons), delta);
                    }));

    py::class_<UCCPAC>(pac_module, "UCCPAC")
            .def_property_readonly("epsilon", &UCCPAC::GetEpsilon)
            .def_property_readonly("delta", &UCCPAC::GetDelta)
            .def_property_readonly("column_indices", &UCCPAC::GetColumnIndices)
            .def_property_readonly("column_names", &UCCPAC::GetColumnNames)
            .def("to_short_string", &UCCPAC::ToShortString)
            .def("to_long_string", &UCCPAC::ToLongString)
            .def("__str__", &UCCPAC::ToLongString)
            .def(py::self == py::self)
            .def("__hash__", [](UCCPAC const& pac) { return py::hash(UCCPACToTuple(pac)); })
            .def(py::pickle(
                    [](UCCPAC const& ucc_pac) {
                        return py::make_tuple(ucc_pac.GetColumnIndices(), ucc_pac.GetColumnNames(),
                                              ucc_pac.GetEpsilon(), ucc_pac.GetDelta());
                    },
                    [](py::tuple t) {
                        if (t.size() != 4) {
                            throw std::runtime_error("Invalid state for UCC PAC pickle");
                        }
                        auto column_indices = t[0].cast<config::IndicesType>();
                        auto column_names = t[1].cast<std::vector<std::string>>();
                        auto epsilon = t[2].cast<double>();
                        auto delta = t[3].cast<double>();
                        return UCCPAC(std::move(column_indices), std::move(column_names), epsilon,
                                      delta);
                    }));

    BindDomains(pac_module);
}
}  // namespace python_bindings
