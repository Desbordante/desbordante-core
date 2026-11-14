#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <boost/container/flat_map.hpp>
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

// inheritance provides aliases to the child class
struct GraphTypes {
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                                        VertexProperties, EdgeProperties>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    using Edge = boost::graph_traits<Graph>::edge_descriptor;
    using Domain = std::unordered_map<Vertex, std::vector<Vertex>>;
    using Mapping = boost::container::flat_map<Vertex, Vertex>;
};

inline bool LabelsMatch(std::string_view lhs, std::string_view rhs) noexcept {
    return lhs == rhs;  // TODO: wildcards
}

}  // namespace model::gdd
