#pragma once

#include <pybind11/pybind11.h>

#include <cstddef>
#include <utility>
#include <vector>

#include <pybind11/pytypes.h>

#include "core/config/custom_metric/custom_metric.h"
#include "core/config/custom_metric/custom_vector_metric.h"
#include "core/model/types/type.h"

namespace python_bindings {
namespace detail {
pybind11::object GetPyObjectFromMixed(model::Type const* type, std::byte const* value);
pybind11::object GetPyObject(model::Type const* type, std::byte const* value);
std::vector<pybind11::object> GetPyObjects(std::vector<model::Type const*> const& types,
                                           std::vector<std::byte const*> const& values);
}  // namespace detail

class PyCustomMetric : public config::ICustomMetric {
private:
    pybind11::object metric_;

public:
    explicit PyCustomMetric(pybind11::object&& metric) : metric_(std::move(metric)) {}

    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override {
        return pybind11::cast<double>(
                metric_(detail::GetPyObject(type, first), detail::GetPyObject(type, second)));
    }
};

class PyCustomVectorMetric : public config::ICustomVectorMetric {
private:
    pybind11::object metric_;

public:
    explicit PyCustomVectorMetric(pybind11::object&& metric) : metric_(std::move(metric)) {}

    double Dist(Types const& types, Values const& first, Values const& second) const override {
        return pybind11::cast<double>(
                metric_(detail::GetPyObjects(types, first), detail::GetPyObjects(types, second)));
    }
};

void BindCustomMetrics(pybind11::module_& data_types_module);
}  // namespace python_bindings
