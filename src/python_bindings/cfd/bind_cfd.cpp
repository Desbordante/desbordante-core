#include <pybind11/pybind11.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include "core/algorithms/cfd/cfun/ccfd.h"
#include "core/algorithms/cfd/mining_algorithms.h"
#include "core/algorithms/cfd/model/raw_cfd.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"
#include "python_bindings/py_util/vector_to_tuple.h"

namespace py = pybind11;

namespace {
py::tuple MakeTableauTuple(algos::cfd::cfun::CCFD const& cfd) {
    auto get_pattern_tuple = [](algos::cfd::cfun::Condition const& cond) {
        return py::make_tuple(python_bindings::VectorToTuple(cond.GetPattern()), cond.GetSupport());
    };
    return python_bindings::VectorToTuple(cfd.GetTableau(), std::move(get_pattern_tuple));
}

py::tuple MakeCfdTuple(algos::cfd::cfun::CCFD const& cfd) {
    auto [lhs, rhs] = cfd.GetEmbeddedFd().ToNameTuple();
    return py::make_tuple(python_bindings::VectorToTuple(lhs), std::move(rhs),
                          MakeTableauTuple(cfd), cfd.GetSupport());
}
}  // namespace

void BindFdFirst(py::module_& cfd_module) {
    using namespace algos::cfd;

    auto fd_first_module = python_bindings::BindPrimitive<FDFirstAlgorithm>(
            cfd_module, &CFDDiscovery::GetCfds, "CfdAlgorithm", "get_cfds", {"FDFirst"});

    py::class_<RawCFD::RawItem>(fd_first_module, "Item")
            .def_property_readonly("attribute", &RawCFD::RawItem::GetAttribute)
            .def_property_readonly("value", &RawCFD::RawItem::GetValue)
            .def(py::pickle(
                    // __getstate__
                    [](RawCFD::RawItem const& item) {
                        return py::make_tuple(item.attribute, item.value);
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for RawItem pickle!");
                        }
                        RawCFD::RawItem item;
                        item.attribute = t[0].cast<AttributeIndex>();
                        item.value = t[1].cast<std::optional<std::string>>();
                        return item;
                    }));

    py::class_<RawCFD>(fd_first_module, "CFD")
            .def("__str__", &RawCFD::ToString)
            .def_property_readonly("lhs_items", &RawCFD::GetLhs)
            .def_property_readonly("rhs_item", &RawCFD::GetRhs)
            .def(py::pickle(
                    // __getstate__
                    [](RawCFD const& cfd) { return py::make_tuple(cfd.GetLhs(), cfd.GetRhs()); },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for CFD pickle!");
                        }
                        auto lhs = t[0].cast<RawCFD::RawItems>();
                        auto rhs = t[1].cast<RawCFD::RawItem>();
                        return RawCFD(std::move(lhs), std::move(rhs));
                    }));
}

void BindCfun(pybind11::module_& cfd_module) {
    using namespace algos::cfd::cfun;
    auto cfun_module = python_bindings::BindPrimitiveNoBase<CFUN>(cfd_module, "CFUN")
                               .def("get_cfds", &CFUN::GetCFDList);

    py::class_<CCFD::Condition>(cfun_module, "Condition")
            .def_property_readonly("pattern", &CCFD::Condition::GetPattern)
            .def_property_readonly("support", &CCFD::Condition::GetSupport)
            .def(py::self == py::self)
            .def("__hash__",
                 [](CCFD::Condition const& cond) {
                     return py::hash(py::make_tuple(
                             python_bindings::VectorToTuple(cond.GetPattern()), cond.GetSupport()));
                 })
            .def(py::pickle(
                    // __getstate__
                    [](CCFD::Condition const& cond) {
                        return py::make_tuple(python_bindings::VectorToTuple(cond.GetPattern()),
                                              cond.GetSupport());
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for Condition pickle!");
                        }

                        auto pattern = t[0].cast<std::vector<std::string>>();
                        size_t support = t[1].cast<std::size_t>();
                        return CCFD::Condition(std::move(pattern), support);
                    }));

    py::class_<CCFD>(cfun_module, "CCFD")
            .def("__str__", &CCFD::ToString)
            .def_property_readonly("embedded_fd", &CCFD::GetEmbeddedFd)
            .def_property_readonly("tableau", &CCFD::GetTableau)
            .def_property_readonly("support", &CCFD::GetSupport)
            .def("__hash__", [](CCFD const& cfd) { return py::hash(MakeCfdTuple(cfd)); })
            .def(py::self == py::self)
            .def(py::pickle(
                    // __getstate__
                    [](CCFD const& cfd) {
                        auto const& embedded_fd = cfd.GetEmbeddedFd();

                        auto schema = table_serialization::SerializeRelationalSchema(
                                embedded_fd.GetSchema().get());
                        auto lhs = table_serialization::SerializeVertical(embedded_fd.GetLhs());
                        auto rhs = table_serialization::SerializeColumn(embedded_fd.GetRhs());
                        auto embedded_fd_tuple =
                                py::make_tuple(std::move(schema), std::move(lhs), std::move(rhs));

                        return py::make_tuple(std::move(embedded_fd_tuple), MakeTableauTuple(cfd),
                                              cfd.GetSupport());
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 3) {
                            throw std::runtime_error("Invalid state for CCFD pickle!");
                        }

                        auto embedded_fd_tuple = t[0].cast<py::tuple>();
                        if (embedded_fd_tuple.size() != 3) {
                            throw std::runtime_error("Invalid state for embedded FD pickle!");
                        }

                        auto schema = table_serialization::DeserializeRelationalSchema(
                                embedded_fd_tuple[0].cast<py::tuple>());
                        auto lhs = table_serialization::DeserializeVertical(
                                embedded_fd_tuple[1].cast<py::tuple>(), schema.get());
                        auto rhs = table_serialization::DeserializeColumn(
                                embedded_fd_tuple[2].cast<py::tuple>(), schema.get());

                        FD embedded_fd(std::move(lhs), std::move(rhs), std::move(schema));

                        auto tableau_tuple = t[1].cast<py::tuple>();
                        CCFD::Tableau tableau;
                        tableau.reserve(tableau_tuple.size());

                        for (size_t i = 0; i < tableau_tuple.size(); ++i) {
                            auto condition_tuple = tableau_tuple[i].cast<py::tuple>();
                            if (condition_tuple.size() != 2) {
                                throw std::runtime_error("Invalid state for embedded FD pickle!");
                            }
                            auto pattern = condition_tuple[0].cast<std::vector<std::string>>();
                            auto support = condition_tuple[1].cast<std::size_t>();

                            tableau.emplace_back(std::move(pattern), support);
                        }

                        auto support = t[2].cast<std::size_t>();

                        return CCFD(std::move(embedded_fd), std::move(tableau), support);
                    }));
}

namespace python_bindings {

void BindCfd(py::module_& main_module) {
    auto cfd_module = main_module.def_submodule("cfd");
    BindCfun(cfd_module);
    BindFdFirst(cfd_module);
}
}  // namespace python_bindings
