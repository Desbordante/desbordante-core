#pragma once

#include <pybind11/pybind11.h>

namespace python_bindings {
void BindPACVerification(pybind11::module_& main_module);
void BindDomainPACVerification(pybind11::module_& pac_verification_module);
void BindFDPACVerification(pybind11::module_& pac_verification_module,
                           pybind11::module_& algorithms_module);
}  // namespace python_bindings
