#include "python_bindings/cfd/bind_cfd.h"

#include <pybind11/pybind11.h>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include "core/algorithms/cfd/cfdfinder/cfdfinder.h"
#include "core/algorithms/cfd/fd_first/fd_first_algorithm.h"
#include "core/algorithms/cfd/model/raw_cfd.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"
#include "python_bindings/py_util/vector_to_tuple.h"

namespace {

namespace py = pybind11;
using algos::cfd::RawCFD;

RawCFD CreateRawCFDFromTuples(py::list lhs_tuples, py::tuple rhs_tuple) {
    RawCFD::RawItems lhs;
    lhs.reserve(lhs_tuples.size());
    for (auto item : lhs_tuples) {
        py::tuple tup = py::cast<py::tuple>(item);
        if (tup.size() != 2) throw py::value_error("LHS items must be (attribute, value) tuples");
        int attr = py::cast<int>(tup[0]);
        std::optional<std::string> value;
        if (!tup[1].is_none()) value = py::cast<std::string>(tup[1]);
        lhs.emplace_back(RawCFD::RawItem{attr, value});
    }

    if (rhs_tuple.size() != 2) throw py::value_error("RHS must be an (attribute, value) tuple");

    int rhs_attr = py::cast<int>(rhs_tuple[0]);
    std::optional<std::string> rhs_value;
    if (!rhs_tuple[1].is_none()) rhs_value = py::cast<std::string>(rhs_tuple[1]);

    return RawCFD(std::move(lhs), RawCFD::RawItem{rhs_attr, rhs_value});
}

py::tuple MakeCfdTuple(algos::cfd::RawCFD const& cfd) {
    RawCFD::RawItems lhs = cfd.GetLhs();
    RawCFD::RawItem rhs = cfd.GetRhs();
    return py::make_tuple(python_bindings::VectorToTuple(lhs), py::cast(rhs));
}

}  // namespace

namespace {
py::tuple MakeCfdTuple(algos::cfdfinder::CFD const& cfd) {
    auto [lhs, rhs] = cfd.GetEmbeddedFD().ToNameTuple();
    auto tableau = cfd.GetTableau();
    return py::make_tuple(
            python_bindings::VectorToTuple(lhs), std::move(rhs),
            python_bindings::VectorToTuple(
                    tableau,
                    [](auto const& pattern) { return python_bindings::VectorToTuple(pattern); }),
            cfd.GetSupport(), cfd.GetConfidence());
}
}  // namespace

namespace python_bindings {
void BindFdFirst(py::module_& cfd_module) {
    using namespace algos::cfd;

    py::class_<RawCFD::RawItem>(cfd_module, "Item")
            .def(py::init<int, std::optional<std::string>>(), py::arg("attribute"),
                 py::arg("value").none(true))
            .def_property_readonly("attribute", &RawCFD::RawItem::GetAttribute)
            .def_property_readonly("value", &RawCFD::RawItem::GetValue)
            .def("__str__", &RawCFD::RawItem::ToString)
            .def("__repr__", &RawCFD::RawItem::ToString)
            .def(py::self == py::self)
            .def("__hash__",
                 [](RawCFD::RawItem const& item) {
                     return py::hash(py::make_tuple(item.GetAttribute(), item.GetValue()));
                 })
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

    py::class_<RawCFD>(cfd_module, "CFD")
            .def(py::init<>())
            .def(py::init<RawCFD::RawItems, RawCFD::RawItem>(), py::arg("lhs"), py::arg("rhs"))
            .def(py::init(&CreateRawCFDFromTuples), py::arg("lhs"), py::arg("rhs"))
            .def("__str__", &RawCFD::ToString)
            .def_property_readonly("lhs", &RawCFD::GetLhs)
            .def_property_readonly("rhs", &RawCFD::GetRhs)
            .def(py::self == py::self)
            .def("__hash__", [](RawCFD const& cfd) { return py::hash(MakeCfdTuple(cfd)); })
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

    BindPrimitive<FDFirstAlgorithm>(cfd_module, &CFDDiscovery::GetCfds, "CfdAlgorithm", "get_cfds",
                                    {"FDFirst"});
}

void BindCfdFinder(pybind11::module_& cfd_module) {
    using namespace algos::cfdfinder;
    auto cfdfinder_module = BindPrimitiveNoBase<CFDFinder>(cfd_module, "CFDFinder")
                                    .def("get_cfds", &CFDFinder::CfdList);

    py::class_<CFD>(cfdfinder_module, "CFD")
            .def("__str__", &CFD::ToString)
            .def_property_readonly("embedded_fd", &CFD::GetEmbeddedFD)
            .def_property_readonly("tableau", &CFD::GetTableau)
            .def_property_readonly("support", &CFD::GetSupport)
            .def_property_readonly("confidence", &CFD::GetConfidence)
            .def("__hash__", [](CFD const& cfd) { return py::hash(MakeCfdTuple(cfd)); })
            .def(py::self == py::self)
            .def(py::pickle(
                    // __getstate__
                    [](CFD const& cfd) {
                        auto const& embedded_fd = cfd.GetEmbeddedFD();

                        auto schema = table_serialization::SerializeRelationalSchema(
                                embedded_fd.GetSchema().get());
                        auto lhs = table_serialization::SerializeVertical(embedded_fd.GetLhs());
                        auto rhs = table_serialization::SerializeColumn(embedded_fd.GetRhs());

                        auto embedded_fd_tuple =
                                py::make_tuple(std::move(schema), std::move(lhs), std::move(rhs));
                        auto tableau = cfd.GetTableau();
                        auto tableau_tuple =
                                python_bindings::VectorToTuple(tableau, [](auto const& pattern) {
                                    return python_bindings::VectorToTuple(pattern);
                                });
                        return py::make_tuple(std::move(embedded_fd_tuple),
                                              std::move(tableau_tuple), cfd.GetSupport(),
                                              cfd.GetConfidence());
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 4) {
                            throw std::runtime_error("Invalid state for CFD pickle!");
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
                        std::vector<std::vector<std::string>> tableau;
                        tableau.reserve(tableau_tuple.size());

                        for (size_t i = 0; i < tableau_tuple.size(); ++i) {
                            auto condition_tuple = tableau_tuple[i].cast<py::tuple>();
                            tableau.push_back(condition_tuple.cast<std::vector<std::string>>());
                        }

                        auto support = t[2].cast<double>();
                        auto confidence = t[3].cast<double>();

                        return CFD(std::move(embedded_fd), std::move(tableau), support, confidence);
                    }));
}

void BindCfd(py::module_& main_module) {
    auto cfd_module = main_module.def_submodule("cfd");
    BindCfdFinder(cfd_module);
    BindFdFirst(cfd_module);
}
}  // namespace python_bindings
