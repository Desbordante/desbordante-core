#pragma once

#include <pybind11/pybind11.h>
#include "core/algorithms/algorithm.h"

namespace configure_algorithm_bind_main_classes {
    void ConfigureAlgo(algos::Algorithm& algorithm, pybind11::kwargs const& kwargs);
}

namespace python_bindings {
void BindMainClasses(pybind11::module_& main_module);
}  // namespace python_bindings
