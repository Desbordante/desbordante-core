#include "bind_od.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/od/fastod/fastod.h"
#include "algorithms/od/fastod/model/canonical_od.h"
#include "algorithms/od/mining_algorithms.h"
#include "algorithms/od/order/list_od.h"
#include "config/indices/type.h"
#include "py_util/bind_primitive.h"
#include "util/bitset_utils.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {

void BindOd(py::module_& main_module) {
    using namespace algos::fastod;
    using namespace algos::order;

    auto od_module = main_module.def_submodule("od");

    py::class_<AscCanonicalOD>(od_module, "AscCanonicalOD")
            .def("__str__", &AscCanonicalOD::ToString)
            .def("__eq__",
                 [](AscCanonicalOD const& od1, AscCanonicalOD const& od2) {
                     return od1.ToString() == od2.ToString();
                 })
            .def("__hash__",
                 [](AscCanonicalOD const& od) { return py::hash(py::str(od.ToString())); })
            .def(py::pickle(
                    // __getstate__
                    [](AscCanonicalOD const& od) {
                        std::vector<int> context_state = od.GetContext().GetColumnIndices();
                        int left = static_cast<int>(od.GetAttributePair().left);
                        int right = static_cast<int>(od.GetAttributePair().right);
                        return py::make_tuple(std::move(context_state), left, right);
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 3) {
                            throw std::runtime_error("Invalid state for AscCanonicalOD pickle!");
                        }
                        std::vector<int> context_state = t[0].cast<std::vector<int>>();
                        int left = t[1].cast<int>();
                        int right = t[2].cast<int>();
                        AttributeSet context = AttributeSet::FromVector(context_state);
                        return AscCanonicalOD(std::move(context), left, right);
                    }));

    py::class_<DescCanonicalOD>(od_module, "DescCanonicalOD")
            .def("__str__", &DescCanonicalOD::ToString)
            .def("__eq__",
                 [](DescCanonicalOD const& od1, DescCanonicalOD const& od2) {
                     return od1.ToString() == od2.ToString();
                 })
            .def("__hash__",
                 [](DescCanonicalOD const& od) { return py::hash(py::str(od.ToString())); })
            .def(py::pickle(
                    // __getstate__
                    [](DescCanonicalOD const& od) {
                        std::vector<int> context_state = od.GetContext().GetColumnIndices();
                        int left = static_cast<int>(od.GetAttributePair().left);
                        int right = static_cast<int>(od.GetAttributePair().right);
                        return py::make_tuple(std::move(context_state), left, right);
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 3) {
                            throw std::runtime_error("Invalid state for DescCanonicalOD pickle!");
                        }
                        std::vector<int> context_state = t[0].cast<std::vector<int>>();
                        int left = t[1].cast<int>();
                        int right = t[2].cast<int>();
                        AttributeSet context = AttributeSet::FromVector(context_state);
                        return DescCanonicalOD(std::move(context), left, right);
                    }));

    py::class_<SimpleCanonicalOD>(od_module, "SimpleCanonicalOD")
            .def("__str__", &SimpleCanonicalOD::ToString)
            .def("__eq__",
                 [](SimpleCanonicalOD const& od1, SimpleCanonicalOD const& od2) {
                     return od1.ToString() == od2.ToString();
                 })
            .def("__hash__",
                 [](SimpleCanonicalOD const& od) { return py::hash(py::str(od.ToString())); })
            .def(py::pickle(
                    // __getstate__
                    [](SimpleCanonicalOD const& od) {
                        std::vector<int> context_state = od.GetContext().GetColumnIndices();
                        int right = static_cast<int>(od.GetRight());
                        return py::make_tuple(std::move(context_state), right);
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for SimpleCanonicalOD pickle!");
                        }
                        std::vector<int> context_state = t[0].cast<std::vector<int>>();
                        int right = t[1].cast<int>();
                        AttributeSet context = AttributeSet::FromVector(context_state);
                        return SimpleCanonicalOD(std::move(context), right);
                    }));

    py::class_<ListOD>(od_module, "ListOD")
            .def_readonly("lhs", &ListOD::lhs)
            .def_readonly("rhs", &ListOD::rhs)
            .def(py::pickle(
                    // __getstate__
                    [](ListOD const& od) { return py::make_tuple(od.lhs, od.rhs); },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for ListOD pickle!");
                        }
                        ListOD od;
                        od.lhs = t[0].cast<std::vector<model::ColumnIndex>>();
                        od.rhs = t[1].cast<std::vector<model::ColumnIndex>>();
                        return od;
                    }));

    auto fastod_algos_module =
            BindPrimitiveNoBase<algos::Fastod>(od_module, "Fastod")
                    .def("get_asc_ods", &algos::Fastod::GetAscendingDependencies)
                    .def("get_desc_ods", &algos::Fastod::GetDescendingDependencies)
                    .def("get_simple_ods", &algos::Fastod::GetSimpleDependencies);
    auto order_algos_module =
            BindPrimitiveNoBase<Order>(od_module, "Order").def("get_list_ods", [](Order& algo) {
                OrderDependencies const& map_res = algo.GetValidODs();
                std::vector<ListOD> res;
                for (auto const& [lhs, rhs_list] : map_res) {
                    for (AttributeList const& rhs : rhs_list) {
                        res.emplace_back(lhs, rhs);
                    }
                }
                return res;
            });

    main_module.attr("od_module") = od_module;
}

}  // namespace python_bindings
