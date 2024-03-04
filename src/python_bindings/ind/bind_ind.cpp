#include "bind_ind.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/ind/ind.h"
#include "py_util/bind_primitive.h"
#include "algorithms/ind/spider/spider.h"
#include "algorithms/ind/faida/faida.h"
#include "algorithms/ind/ind_algorithm.h"

namespace py = pybind11;

namespace python_bindings {
void BindInd(py::module_& main_module) {
    using namespace model;

    auto ind_module = main_module.def_submodule("ind");
    py::class_<IND>(ind_module, "IND")
        .def("__str__", &IND::ToString);
    
    using namespace algos;
    auto ind_algos_module = BindPrimitive<Spider, Faida>(ind_module,
        py::overload_cast<>(&INDAlgorithm::INDList, py::const_), "IndAlgorithm", "get_inds",
        {"Spider", "Faida"});
}
} // namespace python_bindings
