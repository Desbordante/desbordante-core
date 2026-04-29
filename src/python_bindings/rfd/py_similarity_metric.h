#pragma once

#include <pybind11/pybind11.h>

#include "core/algorithms/rfd/similarity_metric.h"

namespace python_bindings {

class PySimilarityMetric : public algos::rfd::SimilarityMetric {
    pybind11::object func_;

public:
    explicit PySimilarityMetric(pybind11::object func) : func_(std::move(func)) {}

    double Compare(std::string const& a, std::string const& b) const override {
        pybind11::gil_scoped_acquire gil;
        return pybind11::cast<double>(func_(a, b));
    }
};

}  // namespace python_bindings
