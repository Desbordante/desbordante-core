#include "ind/bind_ind.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/ind/ind.h"
#include "algorithms/ind/ind_algorithm.h"
#include "algorithms/ind/mining_algorithms.h"
#include "py_util/bind_primitive.h"

namespace py = pybind11;

namespace python_bindings {
void BindInd(py::module_& main_module) {
    using namespace model;

    auto ind_module = main_module.def_submodule("ind");
    py::class_<IND>(ind_module, "IND")
            .def("__str__", &IND::ToLongString)
            .def("to_short_string", &IND::ToShortString)
            .def("to_long_string", &IND::ToLongString)
            .def("get_lhs", &IND::GetLhs)
            .def("get_rhs", &IND::GetRhs);

    using namespace algos;
    auto ind_algos_module = BindPrimitive<Spider, Faida>(
            ind_module, &INDAlgorithm::INDList, "IndAlgorithm", "get_inds", {"Spider", "Faida"});
}
}  // namespace python_bindings
