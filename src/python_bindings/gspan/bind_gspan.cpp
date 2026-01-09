#include "python_bindings/gspan/bind_gspan.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/fsm/gspan/gspan.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {

void BindGSpan(pybind11::module_& main_module) {
    using namespace algos;

    auto gspan_module = main_module.def_submodule("gspan");

    pybind11::class_<gspan::Vertex>(gspan_module, "Vertex")
            .def_readonly("id", &gspan::Vertex::id)
            .def_readonly("label", &gspan::Vertex::label)
            .def("__repr__", [](gspan::Vertex const& v) {
                return "Vertex(id=" + std::to_string(v.id) + ", label=" + std::to_string(v.label) +
                       ")";
            });

    pybind11::class_<gspan::ExtendedEdge>(gspan_module, "Edge")
            .def_readonly("vertex1", &gspan::ExtendedEdge::vertex1)
            .def_readonly("vertex2", &gspan::ExtendedEdge::vertex2)
            .def_readonly("label", &gspan::ExtendedEdge::label)
            .def("__repr__", [](gspan::ExtendedEdge const& e) {
                return "Edge(v1=" + std::to_string(e.vertex1.id) +
                       ", v2=" + std::to_string(e.vertex2.id) +
                       ", label=" + std::to_string(e.label) + ")";
            });

    pybind11::class_<gspan::DFSCode>(gspan_module, "EdgeList")
            .def("get_edges", &gspan::DFSCode::GetExtendedEdges)
            .def("get_vertex_labels", &gspan::DFSCode::GetVertexLabels)
            .def("__len__", &gspan::DFSCode::Size)
            .def("__repr__", &gspan::DFSCode::ToString)
            .def(
                    "__iter__",
                    [](gspan::DFSCode const& self) {
                        return pybind11::make_iterator(self.GetExtendedEdges().begin(),
                                                       self.GetExtendedEdges().end());
                    },
                    pybind11::keep_alive<0, 1>())
            .def("__getitem__", [](gspan::DFSCode const& self, size_t i) {
                if (i >= self.Size()) throw pybind11::index_error("index out of range");
                return self[i];
            });

    pybind11::class_<gspan::FrequentSubgraph>(gspan_module, "FrequentSubgraph")
            .def_readonly("edge_list", &gspan::FrequentSubgraph::dfs_code)
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