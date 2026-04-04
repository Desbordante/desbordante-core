#pragma once

#include <pybind11/pybind11.h>

#include <typeindex>

namespace python_bindings {
pybind11::tuple GetPyType(std::type_index type_index);
}  // namespace python_bindings
