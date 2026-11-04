// bind_cind.cpp
#include "bind_cind.h"

#include <pybind11/pybind11.h>

#include <string>
#include <utility>
#include <vector>

#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "core/algorithms/cind/cind.h"
#include "core/algorithms/cind/cind_algorithm.h"
#include "core/algorithms/cind/condition.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/vector_to_tuple.h"

namespace py = pybind11;

namespace {
using algos::cind::CIND;
using algos::cind::Condition;

std::string PyRepr(py::handle obj) {
    return py::cast<std::string>(py::repr(obj));
}

std::string Repr(Condition const& cond) {
    return "Condition(data=" + PyRepr(python_bindings::VectorToTuple(cond.condition_attrs_values)) +
           ", validity=" + PyRepr(py::float_(cond.validity)) +
           ", completeness=" + PyRepr(py::float_(cond.completeness)) + ")";
}

std::string Repr(CIND const& cind) {
    std::string conditions_repr = "[";
    for (std::size_t i = 0; i < cind.conditions.size(); ++i) {
        if (i != 0) conditions_repr += ", ";
        conditions_repr += Repr(cind.conditions[i]);
    }
    conditions_repr += "]";

    return "CIND(ind=" + PyRepr(py::str(cind.ind.ToLongString())) + ", conditional_attributes=" +
           PyRepr(python_bindings::VectorToTuple(cind.conditional_attributes)) +
           ", conditions=" + conditions_repr + ")";
}
}  // namespace

namespace python_bindings {
void BindCind(py::module_& main_module) {
    using namespace model;
    using namespace algos::cind;

    auto cind_module = main_module.def_submodule("cind");

    py::class_<Condition>(cind_module, "Condition")
            .def(py::init<std::vector<std::string>, double, double>(), py::arg("data"),
                 py::arg("validity"), py::arg("completeness"))
            .def("__str__", &Condition::ToString)
            .def("__repr__", [](Condition const& cond) { return Repr(cond); })
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
            .def(py::init([](IND ind, std::vector<std::string> conditional_attributes,
                             std::vector<Condition> conditions) {
                     return CIND{.ind = std::move(ind),
                                 .conditions = std::move(conditions),
                                 .conditional_attributes = std::move(conditional_attributes)};
                 }),
                 py::arg("ind"), py::arg("conditional_attributes"), py::arg("conditions"))
            .def("__str__", &CIND::ToString)
            .def("__repr__", [](CIND const& cind) { return Repr(cind); })
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
                 [](CIND const& cind) { return VectorToTuple(cind.conditional_attributes); })
            .def(
                    "get_ind", [](CIND const& cind) -> IND const& { return cind.ind; },
                    py::return_value_policy::reference_internal)
            .def("get_ind_string", [](CIND const& cind) { return cind.ind.ToLongString(); });

    BindPrimitiveNoBase<CindAlgorithm>(cind_module, "Cinderella")
            .def("get_cinds", &CindAlgorithm::CINDList);
}
}  // namespace python_bindings
