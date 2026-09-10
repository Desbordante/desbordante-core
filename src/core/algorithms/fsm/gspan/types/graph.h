#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/compressed_sparse_row_graph.hpp>

namespace gspan {

struct Vertex {
    int id = 0;
    int label = 0;
    bool operator==(Vertex const& other) const = default;
};

struct Edge {
    int id = 0;
    int label = 0;
};

struct GraphProps {
    int original_id = 0;
};

using graph_t =
        boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Vertex, Edge, GraphProps>;
using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
using edge_t = boost::graph_traits<graph_t>::edge_descriptor;

using csr_graph_t = boost::compressed_sparse_row_graph<boost::directedS, Vertex, Edge, GraphProps>;
using csr_vertex_t = boost::graph_traits<csr_graph_t>::vertex_descriptor;
using csr_edge_t = boost::graph_traits<csr_graph_t>::edge_descriptor;

}  // namespace gspan