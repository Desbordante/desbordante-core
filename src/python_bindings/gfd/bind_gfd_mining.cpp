#include "gfd/bind_gfd_mining.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/gfd/gfd_miner.h"
#include "py_util/bind_primitive.h"

namespace {}  // namespace

namespace python_bindings {
void BindGfdMining(pybind11::module_& main_module) {
    using namespace algos;

    auto gfd_module = main_module.def_submodule("gfd_mining");

    BindPrimitiveNoBase<GfdMiner>(gfd_module, "GfdMiner").def("get_gfds", &GfdMiner::GfdList);
}
}  // namespace python_bindings
