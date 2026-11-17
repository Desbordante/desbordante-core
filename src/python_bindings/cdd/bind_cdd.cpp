#include "python_bindings/cdd/bind_cdd.h"

#include <pybind11/pybind11.h>

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include "core/algorithms/cdd/cdd.h"

namespace python_bindings {
namespace py = pybind11;

static model::ConditionLimit PyToConditionLimit(py::object val) {
    if (py::isinstance<py::list>(val) || py::isinstance<py::tuple>(val)) {
        auto seq = val.cast<std::vector<py::object>>();
        if (seq.size() == 2 && py::isinstance<py::tuple>(val)) {
            return model::ConditionLimit{
                    std::in_place_type<std::pair<model::ConditionValue, model::ConditionValue>>,
                    std::make_pair(seq[0].cast<model::ConditionValue>(),
                                   seq[1].cast<model::ConditionValue>())};
        }
        std::vector<model::ConditionValue> set;
        set.reserve(seq.size());
        for (auto& item : seq) {
            set.push_back(item.cast<model::ConditionValue>());
        }
        return model::ConditionLimit{std::in_place_type<std::vector<model::ConditionValue>>,
                                     std::move(set)};
    }
    return model::ConditionLimit{std::in_place_type<model::ConditionValue>,
                                 val.cast<model::ConditionValue>()};
}

void BindCDD(py::module_& main_module) {
    auto cdd_module = main_module.def_submodule("cdd");

    py::enum_<model::ConditionOp>(cdd_module, "ConditionOp")
            .value("EQ", model::ConditionOp::EQ)
            .value("NEQ", model::ConditionOp::NEQ)
            .value("LT", model::ConditionOp::LT)
            .value("LE", model::ConditionOp::LE)
            .value("GT", model::ConditionOp::GT)
            .value("GE", model::ConditionOp::GE)
            .value("IN_SET", model::ConditionOp::IN_SET)
            .value("IN_INTERVAL", model::ConditionOp::IN_INTERVAL)
            .value("ANY", model::ConditionOp::ANY);

    py::class_<model::ConditionValue>(cdd_module, "ConditionValue")
            .def(py::init<model::ConditionValue>());

    py::class_<model::Condition>(cdd_module, "Condition")
            .def(py::init([](std::string attr, py::object val, model::ConditionOp op) {
                return model::Condition{attr, PyToConditionLimit(val), op};
            }))
            .def_property_readonly("attribute",
                                   [](model::Condition& self) { return self.attribute_; })
            .def_property_readonly("op", [](model::Condition& self) { return self.op_; })
            .def("__repr__", [](model::Condition& self) { return self.ToString(); })
            .def("__str__", [](model::Condition& self) { return self.ToString(); })
            .def(pybind11::self == pybind11::self)
            .def(pybind11::self != pybind11::self)
            .def("__hash__",
                 [](model::Condition const& cond) { return py::hash(py::str(cond.ToString())); });

    py::class_<model::CDD>(cdd_module, "CDD")
            .def(py::init<model::DDString, std::vector<model::Condition>,
                          std::vector<model::Condition>>(),
                 py::arg("dd"), py::arg("lhs_condition") = std::vector<model::Condition>{},
                 py::arg("rhs_condition") = std::vector<model::Condition>{})
            .def_readwrite("dd", &model::CDD::dd_)
            .def_readwrite("lhs_condition", &model::CDD::lhs_condition_)
            .def_readwrite("rhs_condition", &model::CDD::rhs_condition_)
            .def("__repr__", [](model::CDD& self) { return self.ToString(); })
            .def("__str__", [](model::CDD& self) { return self.ToString(); })
            .def(pybind11::self == pybind11::self)
            .def(pybind11::self != pybind11::self)
            .def("__hash__",
                 [](model::CDD const& cdd) { return py::hash(py::str(cdd.ToString())); });
}

}  // namespace python_bindings
