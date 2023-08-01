#pragma once

#include "algorithms/metric/metric_verifier.h"
#include "get_algorithm.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyMetricVerifier : public PyAlgorithm<algos::metric::MetricVerifier, PyAlgorithmBase> {
    using PyAlgorithmBase::algorithm_;

public:
    [[nodiscard]] bool MfdHolds() const {
        return GetAlgorithm<algos::metric::MetricVerifier>(algorithm_).GetResult();
    }
};

}  // namespace python_bindings
