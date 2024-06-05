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
    using namespace algos;

    static constexpr auto kIndName = "IND";

    auto ind_module = main_module.def_submodule("ind");
    auto ind = py::class_<IND>(ind_module, kIndName)
                       .def("__str__", &IND::ToLongString)
                       .def("to_short_string", &IND::ToShortString)
                       .def("to_long_string", &IND::ToLongString)
                       .def("get_lhs", &IND::GetLhs)
                       .def("get_rhs", &IND::GetRhs);

    BindPrimitive<Faida>(ind_module, &INDAlgorithm::INDList, "IndAlgorithm", "get_inds", {"Faida"});

    auto aind_module = main_module.def_submodule("aind");
    // Currently there is no AIND primitive, using IND instead.
    aind_module.attr(kIndName) = ind;
    BindAlgos<INDAlgorithm, Spider, Mind>(aind_module, {"Spider", "Mind"});
}
}  // namespace python_bindings
