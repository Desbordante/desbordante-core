#include "bind_cind.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/cind/cind_algorithm.h"
#include "algorithms/ind/mining_algorithms.h"
#include "cind/cind_algorithm.h"
#include "py_util/bind_primitive.h"

namespace py = pybind11;

namespace python_bindings {
void BindCind(py::module_& main_module) {
    using namespace model;
    using namespace algos::cind;

    auto cind_module = main_module.def_submodule("cind");
    
    BindPrimitiveNoBase<CindAlgorithm>(cind_module, "Cind")
            .def("get_ainds", &CindAlgorithm::AINDList)
            .def("time_taken", &CindAlgorithm::TimeTaken);
}
}  // namespace python_bindings
