#include "python_bindings/huespan/bind_huespan.h"

#include <string>

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include "core/algorithms/hue/huespan/high_utility_episode.h"
#include "core/algorithms/hue/huespan/huespan.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {

void BindHueSpan(pybind11::module_& main_module) {
    using namespace algos::huespan;

    auto huespan_module = main_module.def_submodule("huespan");

    pybind11::class_<HighUtilityEpisode>(huespan_module, "HighUtilityEpisode")
            .def_property_readonly("utility", &HighUtilityEpisode::GetUtility)
            .def_property_readonly("events", &HighUtilityEpisode::GetEvents)
            .def("__len__", &HighUtilityEpisode::GetSize)
            .def("__repr__", [](HighUtilityEpisode const& episode) {
                std::string result = "HighUtilityEpisode(utility=" +
                                     std::to_string(episode.GetUtility()) + ", events=";
                bool first_event_set = true;
                for (auto const& event_set : episode.GetEvents()) {
                    if (!first_event_set) {
                        result += ", ";
                    }
                    first_event_set = false;
                    result += '(';
                    bool first_event = true;
                    for (int event : event_set) {
                        if (!first_event) {
                            result += ", ";
                        }
                        first_event = false;
                        result += std::to_string(event);
                    }
                    result += ')';
                }
                result += ')';
                return result;
            });

    BindPrimitiveNoBase<Huespan>(huespan_module, "Huespan")
            .def("get_high_utility_episodes", &Huespan::GetHighUtilityEpisodes);
}

}  // namespace python_bindings