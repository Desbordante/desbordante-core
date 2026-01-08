#include "python_bindings/fem/bind_fem.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/fem/maxfem/maxfem.h"
#include "core/model/sequence/composite_episode.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {

namespace {
namespace py = pybind11;
}

void BindFem(pybind11::module_& main_module) {
    using namespace algos::maxfem;

    auto fem_module = main_module.def_submodule("fem");

    py::class_<algos::FEMAlgorithm, algos::Algorithm>(fem_module, "FEMAlgorithm");

    py::class_<model::EventSet>(fem_module, "EventSet")
            .def("get_events",
                 [](model::EventSet const& event_set) { return event_set.GetEvents(); });

    py::class_<model::CompositeEpisode>(fem_module, "Episode")
            .def("get_event_sets",
                 [](model::CompositeEpisode const& episode) { return episode.GetEventSets(); });

    detail::RegisterAlgorithm<MaxFEM, algos::FEMAlgorithm>(fem_module, "MaxFEM")
            .def("get_max_frequent_episodes", &MaxFEM::GetMaxFrequentEpisodes);
}

}  // namespace python_bindings
