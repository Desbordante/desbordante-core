#pragma once

#include <functional>

#include "Metric.h"

template <typename T>
class FuncMetric : public Metric {
private:
    std::function<double(T, T)> metric_function_;

public:
    explicit FuncMetric(std::function<double(T, T)> func) : metric_function_(func) {}

    double Dist(std::byte const* first, std::byte const* second) const override {
        return metric_function_(*reinterpret_cast<T const*>(first),
                                *reinterpret_cast<T const*>(second));
    }
};
