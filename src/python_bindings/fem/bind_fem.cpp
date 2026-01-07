#include "python_bindings/fem/bind_fem.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/fem/maxfem/maxfem.h"
#include "core/algorithms/fem/maxfem/model/parallel_episode.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {

namespace {
namespace py = pybind11;
}

void BindFem(pybind11::module_& main_module) {
    using namespace algos::maxfem;

    auto fem_module = main_module.def_submodule("fem");

    py::class_<algos::FEMAlgorithm, algos::Algorithm>(fem_module, "FEMAlgorithm");

    py::class_<ParallelEpisode>(fem_module, "ParallelEpisode")
            .def("get_support", &ParallelEpisode::GetSupport)
            .def("get_events",
                 [](ParallelEpisode const& episode) { return episode.GetEventSet().GetEvents(); });

    detail::RegisterAlgorithm<MaxFEM, algos::FEMAlgorithm>(fem_module, "MaxFEM")
            .def("get_frequent_episodes", &MaxFEM::GetFrequentEpisodes);
}

}  // namespace python_bindings
