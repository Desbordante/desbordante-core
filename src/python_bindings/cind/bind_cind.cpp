#include "bind_cind.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/ind/cind/cind_algorithm.h"
#include "algorithms/ind/mining_algorithms.h"
#include "ind/cind/cind_algorithm.h"
#include "py_util/bind_primitive.h"

namespace py = pybind11;

namespace python_bindings {
void BindCind(py::module_& main_module) {
    using namespace model;
    using namespace algos::cind;

    auto cind_module = main_module.def_submodule("cind");
    
    BindPrimitiveNoBase<Cind>(cind_module, "Cind")
            .def("get_ainds", &Cind::AINDList)
            .def("time_taken", &Cind::TimeTaken);
}
}  // namespace python_bindings
