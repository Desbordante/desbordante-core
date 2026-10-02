#pragma once

#include <pybind11/pybind11.h>

#include <cstddef>
#include <memory>
#include <string>
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
    class TypedMetric : public ITypedMetric {
    private:
        pybind11::object metric_;
        model::Type const* type_;

    public:
        TypedMetric(pybind11::object metric, model::Type const* type)
            : metric_(std::move(metric)), type_(type) {}

        double operator()(std::byte const* a, std::byte const* b) const override {
            return pybind11::cast<double>(metric_(ValueToPy(type_, a), ValueToPy(type_, b)));
        }
    };

    explicit PyCustomMetric(pybind11::object&& metric) : metric_(std::move(metric)) {}

    std::unique_ptr<ITypedMetric> SetType(model::Type const* type,
                                          std::string const&) const override {
        return std::make_unique<TypedMetric>(metric_, type);
    }
};

class PyCustomVectorMetric : public util::ICustomVectorMetric {
private:
    pybind11::object metric_;

public:
    class TypedMetric : public ITypedMetric {
    private:
        pybind11::object metric_;
        Types types_;

    public:
        TypedMetric(pybind11::object metric, Types const& types)
            : metric_(std::move(metric)), types_(types) {}

        double operator()(Values const& a, Values const& b) const override {
            return pybind11::cast<double>(metric_(ValuesToPy(types_, a), ValuesToPy(types_, b)));
        }
    };

    explicit PyCustomVectorMetric(pybind11::object&& metric) : metric_(std::move(metric)) {}

    std::unique_ptr<ITypedMetric> SetTypes(Types const& types,
                                           std::vector<std::string> const&) const override {
        return std::make_unique<TypedMetric>(metric_, types);
    }
};

void BindCustomMetrics(pybind11::module_& main_module);
}  // namespace python_bindings
