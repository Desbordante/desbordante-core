#pragma once

#include <boost/any.hpp>
#include <pybind11/pybind11.h>

namespace python_bindings {
[[nodiscard]] boost::any PyToAny(std::string_view option_name, std::type_index index,
                                 pybind11::handle obj);
}  // namespace python_bindings
