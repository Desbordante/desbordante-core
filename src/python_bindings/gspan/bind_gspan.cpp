#include "python_bindings/gspan/bind_gspan.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/fsm/gspan/gspan.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {

void BindGSpan(pybind11::module_& main_module) {
    using namespace algos;

    auto gspan_module = main_module.def_submodule("gspan");

    pybind11::class_<gspan::FrequentSubgraph>(gspan_module, "FrequentSubgraph")
            .def_readonly("dfs_code", &gspan::FrequentSubgraph::dfs_code)
            .def_readonly("support", &gspan::FrequentSubgraph::support)
            .def_readonly("graphs_ids", &gspan::FrequentSubgraph::graphs_ids)
            .def("__str__", &gspan::FrequentSubgraph::ToString)
            .def("__hash__",
                 [](gspan::FrequentSubgraph const& self) {
                     return gspan::FrequentSubgraph::Hash{}(self);
                 })
            .def("__eq__", [](gspan::FrequentSubgraph const& self,
                              gspan::FrequentSubgraph const& other) { return self == other; });

    BindPrimitiveNoBase<GSpan>(gspan_module, "GSpan")
            .def("get_frequent_subgraphs", &GSpan::GetFrequentSubgraphs);
}

}  // namespace python_bindings