#include "bind_afd_metric_calculation.h"

#include <pybind11/pybind11.h>

#include "algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
using namespace pybind11::literals;

void BindAfdMetricCalculation(py::module_& main_module) {
    using namespace algos;
    using namespace algos::afd_metric_calculator;

    auto afd_metric_module = main_module.def_submodule("afd_metric_calculation");
    BindPrimitiveNoBase<AFDMetricCalculator>(afd_metric_module, "AFDMetricCalculator")
            .def("get_result", &AFDMetricCalculator::GetResult);
}
}  // namespace python_bindings
