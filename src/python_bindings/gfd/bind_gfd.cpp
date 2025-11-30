#include "python_bindings/gfd/bind_gfd.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/gfd/gfd_miner/gfd_miner.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {
void BindGfd(pybind11::module_& main_module) {
    using namespace algos;

    auto gfd_module = main_module.def_submodule("gfd_mining");

    BindPrimitiveNoBase<GfdMiner>(gfd_module, "GfdMiner").def("get_gfds", &GfdMiner::GfdList);
}
}  // namespace python_bindings
