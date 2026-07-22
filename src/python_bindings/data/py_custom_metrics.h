#pragma once

#include <pybind11/pybind11.h>

#include <cstddef>
#include <utility>

#include <pybind11/pytypes.h>

#include "core/model/types/type.h"
#include "core/util/custom_metric/custom_metric.h"
#include "core/util/custom_metric/custom_vector_metric.h"
#include "python_bindings/py_util/value_to_py.h"

namespace python_bindings {
class PyCustomMetric : public util::ICustomMetric {
private:
    pybind11::object metric_;

public:
    explicit PyCustomMetric(pybind11::object&& metric) : metric_(std::move(metric)) {}

    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override {
        return pybind11::cast<double>(metric_(ValueToPy(type, first), ValueToPy(type, second)));
    }
};

class PyCustomVectorMetric : public util::ICustomVectorMetric {
private:
    pybind11::object metric_;

public:
    explicit PyCustomVectorMetric(pybind11::object&& metric) : metric_(std::move(metric)) {}

    double Dist(Types const& types, Values const& first, Values const& second) const override {
        return pybind11::cast<double>(metric_(ValuesToPy(types, first), ValuesToPy(types, second)));
    }
};

void BindCustomMetrics(pybind11::module_& main_module);
}  // namespace python_bindings
