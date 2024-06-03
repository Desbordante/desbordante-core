#include "ar/bind_ar.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/association_rules/ar.h"
#include "algorithms/association_rules/mining_algorithms.h"
#include "py_util/bind_primitive.h"

namespace py = pybind11;

namespace python_bindings {
void BindAr(py::module_& main_module) {
    using namespace algos;
    using model::ArIDs;
    using model::ARStrings;

    auto ar_module = main_module.def_submodule("ar");
    py::class_<ARStrings>(ar_module, "ARStrings")
            .def("__str__", &ARStrings::ToString)
            .def_readonly("left", &ARStrings::left)
            .def_readonly("right", &ARStrings::right)
            .def_readonly("confidence", &ARStrings::confidence);

    py::class_<ArIDs>(ar_module, "ArIDs")
            .def_readonly("left", &ArIDs::left)
            .def_readonly("right", &ArIDs::right)
            .def_readonly("confidence", &ArIDs::confidence);

    py::class_<ARAlgorithm, Algorithm>(ar_module, "ArAlgorithm")
            .def("get_ars", &ARAlgorithm::GetArStringsList, py::return_value_policy::move)
            .def("get_itemnames", &ARAlgorithm::GetItemNamesVector)
            .def("get_ar_ids", &ARAlgorithm::GetArIDsList);

    BindAlgos<ARAlgorithm, Apriori>(ar_module, {"Apriori"});
}
}  // namespace python_bindings
