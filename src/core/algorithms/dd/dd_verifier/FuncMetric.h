#pragma once

#include <functional>

#include "Metric.h"
#include "core/model/types/type.h"

template <typename T>
class FuncMetric : public Metric {
private:
    std::function<double(T const &, T const &)> metric_function_;

public:
    explicit FuncMetric(std::function<double(T const &, T const &)> func)
        : metric_function_(func) {}

    double Dist(std::byte const *first, std::byte const *second) const override {
        return metric_function_(model::Type::GetValue<T>(first), model::Type::GetValue<T>(second));
    }
};
