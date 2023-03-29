#pragma once

#include "algorithms/statistics/data_stats.h"
#include "py_primitive.h"

namespace python_bindings {

class PyDataStats : public PyPrimitive<algos::DataStats> {
public:
    [[nodiscard]] std::string GetResultString() const {
        return primitive_.ToString();
    }
};

}  // namespace python_bindings
