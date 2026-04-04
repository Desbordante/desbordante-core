#pragma once

#include <pybind11/pybind11.h>

#include <typeindex>

#include <boost/any.hpp>

namespace python_bindings {
pybind11::object OptToPy(std::type_index type, boost::any val);
}  // namespace python_bindings
