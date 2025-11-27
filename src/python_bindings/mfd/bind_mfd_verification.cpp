#include "python_bindings/mfd/bind_mfd_verification.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/metric/highlight.h"
#include "core/algorithms/metric/verification_algorithms.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindMfdVerification(py::module_& main_module) {
    using namespace algos;
    using namespace algos::metric;

    auto mfd_module = main_module.def_submodule("mfd_verification");
    py::class_<Highlight>(mfd_module, "Highlight")
            .def("to_tuple", &Highlight::ToTuple)
            .def_readonly("data_index", &Highlight::data_index)
            .def_readonly("furthest_data_index", &Highlight::furthest_data_index)
            .def_readonly("max_distance", &Highlight::max_distance);
    BindPrimitiveNoBase<MetricVerifier>(mfd_module, "MetricVerifier")
            .def("get_highlights", &MetricVerifier::GetHighlights)
            .def("mfd_holds", &MetricVerifier::GetResult);
}
}  // namespace python_bindings
