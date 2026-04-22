#pragma once

#include <pybind11/pybind11.h>

namespace python_bindings {
void BindGddVerification(pybind11::module_& main_module);
}  // namespace python_bindings
