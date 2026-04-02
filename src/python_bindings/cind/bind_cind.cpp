// bind_cind.cpp
#include "bind_cind.h"

#include <pybind11/pybind11.h>

#include <vector>

#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "algorithms/cind/cind.h"
#include "algorithms/cind/cind_algorithm.h"
#include "algorithms/cind/condition.h"
#include "algorithms/ind/mining_algorithms.h"
#include "py_util/bind_primitive.h"

namespace py = pybind11;

namespace {
template <typename ElementType>
py::tuple VectorToTuple(std::vector<ElementType> vec) {
    std::size_t const size = vec.size();
    py::tuple tuple(size);
    for (std::size_t i = 0; i < size; ++i) {
        tuple[i] = std::move(vec[i]);
    }
    return tuple;
}
}  // namespace

namespace python_bindings {
void BindCind(py::module_& main_module) {
    using namespace model;
    using namespace algos::cind;

    auto cind_module = main_module.def_submodule("cind");

    py::class_<Condition>(cind_module, "Condition")
            .def("__str__", &Condition::ToString)
            .def("__eq__",
                 [](Condition const& cond1, Condition const& cond2) { return cond1 == cond2; })
            .def("__hash__",
                 [](Condition const& cond) {
                     return py::hash(py::int_(std::hash<Condition>{}(cond)));
                 })
            .def("data",
                 [](Condition const& cond) { return VectorToTuple(cond.condition_attrs_values); })
            .def("validity", [](Condition const& cond) { return cond.validity; })
            .def("completeness", [](Condition const& cond) { return cond.completeness; })
            .def(py::pickle(
                    // __getstate__
                    [](Condition const& cond) {
                        return py::make_tuple(cond.condition_attrs_values, cond.validity,
                                              cond.completeness);
                    },
                    // __setstate__
                    [](py::tuple st) {
                        if (st.size() != 3) {
                            throw std::runtime_error("Invalid state for Condition pickle!");
                        }
                        return Condition(st[0].cast<std::vector<std::string>>(),
                                         st[1].cast<double>(), st[2].cast<double>());
                    }));

    py::class_<CIND>(cind_module, "CIND")
            .def("__str__", &CIND::ToString)
            .def("__eq__", [](CIND const& cind1, CIND const& cind2) { return cind1 == cind2; })
            .def("__hash__", [](CIND const& cind) { return py::hash(py::int_(cind.Hash())); })
            .def("conditions_number", &CIND::ConditionsNumber)
            .def("get_conditions",
                 [](CIND const& cind) {
                     py::list result;
                     for (auto const& cond : cind.conditions) {
                         result.append(cond);
                     }
                     return result;
                 })
            .def("get_condition_attributes",
                 [](CIND const& cind) { return VectorToTuple(cind.conditional_attributes); });

    BindPrimitiveNoBase<CindAlgorithm>(cind_module, "Cinderella")
            .def("get_cinds", &CindAlgorithm::CINDList);
}
}  // namespace python_bindings
