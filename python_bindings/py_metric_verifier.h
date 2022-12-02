#pragma once

#include "algorithms/metric/metric_verifier.h"
#include "py_primitive.h"

namespace python_bindings {

class PyMetricVerifier : public PyPrimitive<algos::metric::MetricVerifier> {
public:
    bool GetResults() const {
        return primitive_.GetResult();
    }
};

}  // namespace python_bindings
