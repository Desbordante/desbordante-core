#pragma once

#include <boost/any.hpp>
#include <pybind11/pybind11.h>

namespace python_bindings {
[[nodiscard]] boost::any PyToAny(std::type_index index, pybind11::object const& obj);
}  // namespace python_bindings
