#pragma once

#include "algorithms/statistics/csv_stats.h"
#include "py_primitive.h"

namespace python_bindings {

class PyCsvStats : public PyPrimitive<algos::CsvStats> {
public:
    [[nodiscard]] std::string GetResults() const {
        return primitive_.ToString();
    }
};

}  // namespace python_bindings
