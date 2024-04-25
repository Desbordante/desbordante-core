#include "ar/bind_ar.h"

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
            .def("get_ars", &ARAlgorithm::GetArStringsList,
                 py::return_value_policy::reference_internal)
            .def("get_itemnames", &ARAlgorithm::GetItemNamesVector)
            .def("get_ar_ids", &ARAlgorithm::GetArIDsList);

    auto algos_module = ar_module.def_submodule("algorithms");
    auto default_algorithm =
            detail::RegisterAlgorithm<Apriori, ARAlgorithm>(algos_module, "Apriori");
    algos_module.attr("Default") = default_algorithm;

    // Perhaps in the future there will be a need for:
    // default_algorithm.def("get_frequent_list", &Apriori::GetFrequentList);
}
}  // namespace python_bindings
