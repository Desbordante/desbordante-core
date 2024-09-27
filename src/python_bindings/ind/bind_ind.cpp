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

    auto ind_module = main_module.def_submodule("ind");
    py::class_<IND>(ind_module, "IND")
            .def("__str__", &IND::ToLongString)
            .def("to_short_string", &IND::ToShortString)
            .def("to_long_string", &IND::ToLongString)
            .def("get_lhs", &IND::GetLhs)
            .def("get_rhs", &IND::GetRhs)
            .def("get_error", &IND::GetError);

    static constexpr auto kSpiderName = "Spider";
    static constexpr auto kMindName = "Mind";

    auto ind_algos_module =
            BindPrimitive<Spider, Faida, Mind>(ind_module, &INDAlgorithm::INDList, "IndAlgorithm",
                                               "get_inds", {kSpiderName, "Faida", kMindName});
    auto define_submodule = [&ind_algos_module, &main_module](char const* name,
                                                              std::vector<char const*> algorithms) {
        auto algos_module = main_module.def_submodule(name).def_submodule("algorithms");
        for (auto algo_name : algorithms) {
            algos_module.attr(algo_name) = ind_algos_module.attr(algo_name);
        }
        algos_module.attr("Default") = algos_module.attr(algorithms.front());
    };

    define_submodule("aind", {kSpiderName, kMindName});
}
}  // namespace python_bindings
