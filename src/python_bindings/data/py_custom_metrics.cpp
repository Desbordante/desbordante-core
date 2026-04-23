#include "python_bindings/data/py_custom_metrics.h"

#include <pybind11/pybind11.h>

#include <cassert>
#include <memory>

#include <pybind11/pytypes.h>

#include "core/util/custom_metric/custom_metric.h"
#include "core/util/custom_metric/custom_vector_metric.h"

namespace py = pybind11;

namespace python_bindings {
void BindCustomMetrics(pybind11::module_& main_module) {
    auto metrics_module = main_module.def_submodule("metrics");

    // These classes are used only in get_opts
    py::class_<util::ICustomMetric, std::shared_ptr<util::ICustomMetric>>(metrics_module,
                                                                          "CustomMetric");
    py::class_<util::ICustomVectorMetric, std::shared_ptr<util::ICustomVectorMetric>>(
            metrics_module, "CustomVectorMetrics");
}
}  // namespace python_bindings
