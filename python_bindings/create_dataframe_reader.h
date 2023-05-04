#pragma once

#include <pybind11/pybind11.h>

#include "algorithms/relational_algorithm.h"

namespace python_bindings {
algos::RelationStream CreateDataFrameReader(pybind11::handle dataframe,
                                            std::string name = "Pandas dataframe");
}  // namespace python_bindings
