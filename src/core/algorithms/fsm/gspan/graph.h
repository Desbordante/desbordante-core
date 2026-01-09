#pragma once

#include <unordered_map>

#include <boost/graph/adjacency_list.hpp>

namespace gspan {

struct Vertex {
    int id = 0;
    int label = 0;
    bool operator==(Vertex const& other) const = default;
};

struct Edge {
    int label = 0;
};

struct GraphProps {
    int id = 0;
    std::unordered_map<int, std::vector<size_t>> label_to_vertices;
};

using graph_t = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, Vertex, Edge,
                                      GraphProps>;

using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
using edge_t = boost::graph_traits<graph_t>::edge_descriptor;

}  // namespace gspan