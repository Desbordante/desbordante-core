#pragma once

#include <pybind11/pybind11.h>

#include "core/config/tabular_data/input_table_type.h"

namespace python_bindings {
config::InputTable CreateDataFrameReader(pybind11::handle dataframe,
                                         std::string name = "Pandas dataframe");
}  // namespace python_bindings
