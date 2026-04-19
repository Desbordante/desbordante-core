#pragma once

#include <pybind11/pybind11.h>

namespace python_bindings {

void BindGaRfd(pybind11::module_& m);

}  // namespace python_bindings