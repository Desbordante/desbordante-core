#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/nar/mining_algorithms.h"
#include "algorithms/nar/nar.h"
#include "algorithms/nar/value_range.h"
#include "py_util/bind_primitive.h"

namespace python_bindings {

namespace py = pybind11;

void BindNar(py::module_& main_module) {
    using namespace algos;
    using namespace algos::des;
    using namespace model;

    auto nar_module = main_module.def_submodule("nar");

    py::class_<ValueRange, std::shared_ptr<ValueRange>>(nar_module, "ValueRange")
            .def("__str__", &ValueRange::ToString)
            .def_property_readonly("type", &ValueRange::GetTypeId);

    py::class_<StringValueRange, ValueRange, std::shared_ptr<StringValueRange>>(nar_module,
                                                                                "StringValueRange")
            .def("__str__", &StringValueRange::ToString)
            .def_readonly("string", &StringValueRange::domain);

    py::class_<NumericValueRange<Double>, ValueRange, std::shared_ptr<NumericValueRange<Double>>>(
            nar_module, "FloatValueRange")
            .def("__str__", &NumericValueRange<Double>::ToString)
            .def_readonly("lower_bound", &NumericValueRange<Double>::lower_bound)
            .def_readonly("upper_bound", &NumericValueRange<Double>::upper_bound);

    py::class_<NumericValueRange<Int>, ValueRange, std::shared_ptr<NumericValueRange<Int>>>(
            nar_module, "IntValueRange")
            .def("__str__", &NumericValueRange<Int>::ToString)
            .def_readonly("lower_bound", &NumericValueRange<Int>::lower_bound)
            .def_readonly("upper_bound", &NumericValueRange<Int>::upper_bound);

    py::class_<NARQualities>(nar_module, "NarQualities")
            .def("__str__", &NARQualities::ToString)
            .def_readonly("support", &NARQualities::support)
            .def_readonly("confidence", &NARQualities::confidence)
            .def_readonly("fitness", &NARQualities::fitness);

    py::class_<NAR>(nar_module, "NAR")
            .def("__str__", &NAR::ToString)
            .def_property_readonly("qualities", &NAR::GetQualities)
            .def_property_readonly("support", [](NAR const& n) { return n.GetQualities().support; })
            .def_property_readonly("confidence",
                                   [](NAR const& n) { return n.GetQualities().confidence; })
            .def_property_readonly("fitness", [](NAR const& n) { return n.GetQualities().fitness; })
            .def_property_readonly("ante", &NAR::GetAnte)
            .def_property_readonly("cons", &NAR::GetCons);

    BindPrimitive<DES>(nar_module, &NARAlgorithm::GetNARVector, "NarAlgorithm", "get_nars", {"DES"},
                       pybind11::return_value_policy::copy);
}
}  // namespace python_bindings
