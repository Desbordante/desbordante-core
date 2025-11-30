#include "python_bindings/cfd/bind_cfd.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/cfd/fd_first_algorithm.h"
#include "core/algorithms/cfd/model/raw_cfd.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace py = pybind11;

namespace python_bindings {
void BindCfd(py::module_& main_module) {
    using namespace algos::cfd;

    auto cfd_module = main_module.def_submodule("cfd");

    py::class_<RawCFD::RawItem>(cfd_module, "Item")
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

    py::class_<RawCFD>(cfd_module, "CFD")
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

    BindPrimitive<FDFirstAlgorithm>(cfd_module, &CFDDiscovery::GetCfds, "CfdAlgorithm", "get_cfds",
                                    {"FDFirst"});
}
}  // namespace python_bindings
