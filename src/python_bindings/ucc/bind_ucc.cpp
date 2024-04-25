#include "ucc/bind_ucc.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/ucc/mining_algorithms.h"
#include "algorithms/ucc/ucc.h"
#include "config/indices/type.h"
#include "py_util/bind_primitive.h"
#include "util/bitset_utils.h"

namespace {
namespace py = pybind11;
using model::UCC;
}  // namespace

namespace python_bindings {
void BindUcc(py::module_& main_module) {
    using namespace algos;

    auto ucc_module = main_module.def_submodule("ucc");
    py::class_<UCC>(ucc_module, "UCC")
            .def("__str__", &UCC::ToIndicesString)
            .def_property_readonly("indices", &UCC::GetColumnIndicesAsVector);
    BindPrimitive<HyUCC, PyroUCC>(ucc_module,
                                  py::overload_cast<>(&UCCAlgorithm::UCCList, py::const_),
                                  "UccAlgorithm", "get_uccs", {"HyUCC", "PyroUCC"});
}
}  // namespace python_bindings
