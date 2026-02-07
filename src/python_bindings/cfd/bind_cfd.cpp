#include "python_bindings/cfd/bind_cfd.h"

#include <cstddef>

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/cfd/fd_first_algorithm.h"
#include "core/algorithms/cfd/model/raw_cfd.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {

namespace py = pybind11;
using algos::cfd::RawCFD;

RawCFD CreateRawCFDFromTuples(py::list lhs_tuples, py::tuple rhs_tuple) {
    RawCFD::RawItems lhs;
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

template <typename ElementType>
py::tuple VectorToTuple(std::vector<ElementType> vec) {
    std::size_t const size = vec.size();
    py::tuple tuple(size);
    for (std::size_t i = 0; i < size; ++i) {
        tuple[i] = std::move(vec[i]);
    }
    return tuple;
}

py::tuple MakeCfdTuple(algos::cfd::RawCFD const& cfd) {
    RawCFD::RawItems lhs = cfd.GetLhs();
    RawCFD::RawItem rhs = cfd.GetRhs();
    return py::make_tuple(VectorToTuple(std::move(lhs)), py::cast(rhs));
}

}  // namespace

namespace python_bindings {
void BindCfd(py::module_& main_module) {
    using namespace algos::cfd;

    auto cfd_module = main_module.def_submodule("cfd");

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
}  // namespace python_bindings
