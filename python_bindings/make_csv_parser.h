#pragma once

#include <pybind11/pybind11.h>

#include "algorithms/relational_algorithm.h"
#include "parser/csv_parser.h"

namespace python_bindings {
template <typename... Args>
algos::RelationStream MakeCsvParser(Args... args) {
    return std::make_shared<CSVParser>(std::forward<Args>(args)...);
}
}  // namespace python_bindings
