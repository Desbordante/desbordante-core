#pragma once

#include "algorithms/statistics/data_stats.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyDataStats : public PyAlgorithm<algos::DataStats, PyAlgorithmBase> {
    using PyAlgorithm<algos::DataStats, PyAlgorithmBase>::GetAlgorithm;

public:
    [[nodiscard]] std::string GetResultString() const {
        return GetAlgorithm().ToString();
    }
};

}  // namespace python_bindings
