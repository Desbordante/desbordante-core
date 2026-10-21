#include "python_bindings/fd/bind_afd.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/fd/afd.h"
#include "core/algorithms/fd/afd_algorithm.h"
#include "core/algorithms/fd/tane/pfdtane.h"
#include "core/algorithms/fd/tane/tane.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"

namespace python_bindings {
void BindAfd(py::module_& main_module) {
    using namespace algos;

    auto afd_module = main_module.def_submodule("afd");
    py::class_<AFD, FD>(afd_module, "AFD")
            .def("get_threshold", &AFD::GetThreshold)
            .def(py::pickle(
                    // __getstate__
                    [](AFD const& afd) {
                        py::tuple schema_state = table_serialization::SerializeRelationalSchema(
                                afd.GetSchema().get());
                        py::tuple lhs_state = table_serialization::SerializeVertical(afd.GetLhs());
                        py::tuple rhs_state = table_serialization::SerializeColumn(afd.GetRhs());
                        return py::make_tuple(std::move(schema_state), std::move(lhs_state),
                                              std::move(rhs_state), afd.GetThreshold());
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 4)
                            throw std::runtime_error("Invalid state for AFD pickle!");
                        std::shared_ptr<RelationalSchema const> schema =
                                table_serialization::DeserializeRelationalSchema(
                                        t[0].cast<py::tuple>());
                        Vertical lhs = table_serialization::DeserializeVertical(
                                t[1].cast<py::tuple>(), schema.get());
                        Column rhs = table_serialization::DeserializeColumn(t[2].cast<py::tuple>(),
                                                                            schema.get());
                        long double threshold = t[3].cast<long double>();
                        return AFD(lhs, rhs, threshold, std::move(schema));
                    }));

    static constexpr auto kTaneName = "Tane";
    static constexpr auto kPFDTaneName = "PFDTane";
    static constexpr auto kPyroName = "Pyro";
    auto afd_algos_module =
            BindPrimitive<Tane, PFDTane>(afd_module, &AFDAlgorithm::SortedAfdList, "AfdAlgorithm",
                                         "get_fds", {kTaneName, kPFDTaneName});

    // Pyro is registered as an FdAlgorithm in bind_fd.cpp: it discovers approximate FDs via a
    // g1 threshold but returns plain FD objects (no threshold accessor), unlike Tane/PFDTane,
    // which return AFD objects. Re-registering the Pyro class here would abort at import time
    // (pybind11 does not allow the same C++ type to be bound twice), so it is aliased instead:
    // this reuses the single Python class already created in bind_fd.cpp and only exposes it
    // under an additional path, so that it is discoverable alongside the other AFD-discovery
    // algorithms.
    afd_algos_module.attr(kPyroName) = main_module.attr("fd").attr("algorithms").attr(kPyroName);

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
