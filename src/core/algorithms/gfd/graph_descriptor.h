#pragma once

#include <map>
#include <string>

#include <boost/graph/adjacency_list.hpp>

struct Vertex {
    int node_id;
    std::map<std::string, std::string> attributes;
};

struct Edge {
    std::string label;
};

using graph_t = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, Vertex, Edge>;
using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
using edge_t = boost::graph_traits<graph_t>::edge_descriptor;
