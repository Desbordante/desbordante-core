#include "python_bindings/fd/bind_afd.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/fd/afd.h"
#include "core/algorithms/fd/afd_algorithm.h"
#include "core/algorithms/fd/mining_algorithms.h"
#include "core/config/indices/type.h"
#include "core/util/bitset_utils.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"

namespace python_bindings {
void BindAfd(py::module_& main_module) {
    using namespace algos;

    auto afd_module = main_module.def_submodule("afd");
    py::class_<AFD, FD>(afd_module, "AFD").def("get_threshold", &AFD::GetThreshold);

    static constexpr auto kTaneName = "Tane";
    static constexpr auto kPFDTaneName = "PFDTane";
    auto afd_algos_module = BindPrimitive<Tane, PFDTane>(
            afd_module, &AFDAlgorithm::SortedAfdList, "AfdAlgorithm", "get_afds",
            {kTaneName, kPFDTaneName}, pybind11::return_value_policy::copy);

    auto define_submodule = [&afd_algos_module, &main_module](char const* name,
                                                              std::vector<char const*> algorithms) {
        auto algos_module = main_module.def_submodule(name).def_submodule("algorithms");
        for (auto algo_name : algorithms) {
            algos_module.attr(algo_name) = afd_algos_module.attr(algo_name);
        }
        algos_module.attr("Default") = algos_module.attr(algorithms.front());
    };

    define_submodule("pfd", {kPFDTaneName});
}
}  // namespace python_bindings
