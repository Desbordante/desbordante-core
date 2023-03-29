#pragma once

#include <typeindex>

#include <pybind11/pybind11.h>

namespace python_bindings {
pybind11::tuple GetPyType(std::type_index type_index);
}  // namespace python_bindings
