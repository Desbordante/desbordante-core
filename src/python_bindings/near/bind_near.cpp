#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/near/mining_algorithms.h"
#include "algorithms/near/near.h"
#include "py_util/bind_primitive.h"

namespace python_bindings {

namespace py = pybind11;

void BindNear(py::module_& main_module) {
    using namespace algos;
    using namespace model;

    auto near_module = main_module.def_submodule("near");

    py::class_<NeARIDs>(near_module, "NeAR");

    BindPrimitive<Kingfisher>(near_module, &NeARDiscovery::GetNeARIDsVector, "NearDiscovery", "get_nears", {"Kingfisher"},
                       pybind11::return_value_policy::copy);
}
}  // namespace python_bindings
