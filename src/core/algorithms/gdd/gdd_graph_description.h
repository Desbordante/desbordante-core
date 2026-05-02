#pragma once

#include <string>
#include <unordered_map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_selectors.hpp>

namespace model::gdd {

struct VertexProperties {
    std::size_t id;
    std::string label;
    std::unordered_map<std::string, std::string> attributes;
};

struct EdgeProperties {
    std::string label;
};

using graph_t = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                                      VertexProperties, EdgeProperties>;
using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
using edge_t = boost::graph_traits<graph_t>::edge_descriptor;

}  // namespace model::gdd
