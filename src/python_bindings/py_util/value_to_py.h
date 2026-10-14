#pragma once

#include <pybind11/pybind11.h>

#include <cstddef>
#include <vector>

#include "core/model/types/type.h"

namespace python_bindings {
pybind11::object ValueToPy(model::Type const* type, std::byte const* value);
std::vector<pybind11::object> ValuesToPy(std::vector<model::Type const*> const& types,
                                         std::vector<std::byte const*> const& values);
}  // namespace python_bindings
