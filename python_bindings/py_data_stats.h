#pragma once

#include "algorithms/statistics/data_stats.h"
#include "get_algorithm.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyDataStats : public PyAlgorithm<algos::DataStats, PyAlgorithmBase> {
    using PyAlgorithmBase::algorithm_;

public:
    [[nodiscard]] std::string GetResultString() const {
        return GetAlgorithm<algos::DataStats>(algorithm_).ToString();
    }
};

}  // namespace python_bindings
