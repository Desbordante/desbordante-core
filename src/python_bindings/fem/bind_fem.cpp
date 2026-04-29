#include "python_bindings/fem/bind_fem.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/fem/afem/afem.h"
#include "core/algorithms/fem/maxfem/maxfem.h"
#include "core/algorithms/fem/tke/tke.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {

namespace {
namespace py = pybind11;
}

void BindFem(pybind11::module_& main_module) {
    auto fem_module = main_module.def_submodule("fem");

    py::class_<algos::FEMAlgorithm, algos::Algorithm>(fem_module, "FEMAlgorithm");

    detail::RegisterAlgorithm<algos::maxfem::MaxFEM, algos::FEMAlgorithm>(fem_module, "MaxFEM")
            .def("get_max_frequent_episodes", &algos::maxfem::MaxFEM::GetMaxFrequentEpisodes);

    detail::RegisterAlgorithm<algos::afem::AFEM, algos::FEMAlgorithm>(fem_module, "AFEM")
            .def("get_frequent_episodes", &algos::afem::AFEM::GetFrequentEpisodes);

    detail::RegisterAlgorithm<algos::tke::TKE, algos::FEMAlgorithm>(fem_module, "TKE")
            .def("get_top_k_frequent_episodes", &algos::tke::TKE::GetTopKFrequentEpisodes);
}

}  // namespace python_bindings
