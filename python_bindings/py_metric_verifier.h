#pragma once

#include "algorithms/metric/metric_verifier.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyMetricVerifier : public PyAlgorithm<algos::metric::MetricVerifier, PyAlgorithmBase> {
    using PyAlgorithm<algos::metric::MetricVerifier, PyAlgorithmBase>::GetAlgorithm;

public:
    [[nodiscard]] bool MfdHolds() const {
        return GetAlgorithm().GetResult();
    }
};

}  // namespace python_bindings
