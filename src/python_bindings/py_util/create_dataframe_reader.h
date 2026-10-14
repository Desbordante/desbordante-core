#pragma once

#include <pybind11/pybind11.h>

#include "core/config/tabular_data/input_table_type.h"
#include "core/model/index.h"

namespace python_bindings {
static constexpr model::Index kSingleDataFrame = -1;

config::InputTable CreateDataFrameReader(pybind11::handle dataframe,
                                         model::Index object_index = kSingleDataFrame);
}  // namespace python_bindings
