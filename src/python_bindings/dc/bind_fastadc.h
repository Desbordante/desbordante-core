#pragma once

#include <pybind11/pybind11.h>

namespace python_bindings {
void BindFastADC(pybind11::module_& main_module);
}  // namespace python_bindings
