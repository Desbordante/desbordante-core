#include "bind_ar.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/association_rules/ar.h"
#include "algorithms/association_rules/mining_algorithms.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindAr(py::module_& main_module) {
    using namespace algos;
    using model::ARStrings;

    auto ar_module = main_module.def_submodule("ar");
    py::class_<ARStrings>(ar_module, "AssociativeRule")
            .def("__str__", &ARStrings::ToString)
            .def_readonly("left", &ARStrings::left)
            .def_readonly("right", &ARStrings::right)
            .def_readonly("confidence", &ARStrings::confidence);
    BindPrimitive<Apriori>(ar_module, &ARAlgorithm::GetArStringsList, "ArAlgorithm", "get_ars",
                           {"Apriori"}, py::return_value_policy::move);
}
}  // namespace python_bindings
