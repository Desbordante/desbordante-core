#include "python_bindings/pac/bind_pac.h"

#include <pybind11/pybind11.h>

#include <cstdlib>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <pybind11/detail/common.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "core/algorithms/pac/domain_pac.h"
#include "core/algorithms/pac/fd_pac.h"
#include "core/model/table/column.h"
#include "core/model/table/vertical.h"
#include "python_bindings/pac/bind_domains.h"
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
    using namespace pybind11::literals;
    using namespace table_serialization;

    auto pac_module = main_module.def_submodule("pac");

    // Domain PACs cannot be pickled, because they contain user-defined metrics
    py::class_<DomainPAC>(pac_module, "DomainPAC")
            .def_property_readonly("epsilon", &DomainPAC::GetEpsilon)
            .def_property_readonly("delta", &DomainPAC::GetDelta)
            .def_property_readonly("domain", &DomainPAC::GetDomainName)
            .def_property_readonly("column_indices",
                                   [](model::DomainPAC const& d_pac) {
                                       return d_pac.GetColumns().GetColumnIndicesAsVector();
                                   })
            .def_property_readonly("column_names", &model::DomainPAC::GetColumnNames)
            .def("to_short_string", &DomainPAC::ToShortString)
            .def("to_long_string", &DomainPAC::ToLongString)
            .def("__str__", &DomainPAC::ToLongString)
            .def(py::self == py::self)
            .def("__hash__",
                 [](DomainPAC const& d_pac) { return py::hash(DomainPACToTuple(d_pac)); });

    auto get_col_names = [](Vertical const& vert) {
        std::vector<std::string> col_names;
        col_names.reserve(vert.GetArity());
        std::ranges::transform(vert.GetColumns(), std::back_inserter(col_names),
                               [](Column const* col) { return col->GetName(); });
        return col_names;
    };
    py::class_<FDPAC>(pac_module, "FDPAC")
            .def_property_readonly("epsilons", &FDPAC::GetEpsilons)
            .def_property_readonly("delta", &FDPAC::GetDelta)
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
            .def("to_short_string", &FDPAC::ToShortString)
            .def("to_long_string", &FDPAC::ToLongString)
            .def("__str__", &FDPAC::ToLongString)
            .def(py::self == py::self)
            .def("__hash__", [](FDPAC const& fd_pac) { return py::hash(FDPAcToTuple(fd_pac)); })
            .def(py::pickle(
                    // __getstate__
                    [](FDPAC const& fd_pac) {
                        auto const* schema = fd_pac.GetLhs().GetSchema();
                        auto schema_state = SerializeRelationalSchema(schema);
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
                        return FDPAC(std::move(lhs), std::move(rhs), std::move(lhs_deltas),
                                     std::move(epsilons), delta);
                    }));

    BindDomains(pac_module);
}
}  // namespace python_bindings
