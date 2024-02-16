#pragma once

#include <typeindex>

#include <boost/any.hpp>
#include <pybind11/pybind11.h>

namespace python_bindings {
pybind11::object OptToPy(std::type_index type, boost::any val);
}  // namespace python_bindings
