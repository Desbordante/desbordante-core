#pragma once
#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

namespace python_bindings {

void BindCDD(pybind11::module_& main_module);

}  // namespace python_bindings
