#include "backtracking_graph_matcher.h"

#include <algorithm>

#include <boost/range/iterator_range.hpp>

namespace algos::detail {

bool BacktrackingCore::GraphHasCompatibleEdge(Graph const& graph, Vertex graph_src,
                                              Vertex graph_dst,
                                              std::string const& pattern_edge_label) {
    for (auto const graph_edge : boost::make_iterator_range(boost::out_edges(graph_src, graph))) {
        if (boost::target(graph_edge, graph) != graph_dst) {
            continue;
        }

        if (model::gdd::LabelsMatch(pattern_edge_label, graph[graph_edge].label)) {
            return true;
        }
    }

    return false;
}

bool BacktrackingCore::AllPatternEdgesArePreserved(Graph const& graph, Graph const& pattern,
                                                   Vertex pattern_src, Vertex pattern_dst,
                                                   Vertex graph_src, Vertex graph_dst) {
    for (auto const pattern_edge :
         boost::make_iterator_range(boost::out_edges(pattern_src, pattern))) {
        if (boost::target(pattern_edge, pattern) != pattern_dst) {
            continue;
        }

        if (!GraphHasCompatibleEdge(graph, graph_src, graph_dst, pattern[pattern_edge].label)) {
            return false;
        }
    }

    return true;
}

bool BacktrackingCore::CanExtendMapping(Graph const& graph, Mapping const& partial_map,
                                        Graph const& pattern, Vertex pattern_var,
                                        Vertex graph_vertex, bool injective) {
    return std::ranges::all_of(partial_map, [&](auto const& mapped_pair) {
        auto const& [mapped_pattern_var, mapped_graph_vertex] = mapped_pair;

        if (injective && mapped_graph_vertex == graph_vertex) {
            return false;
        }

        return AllPatternEdgesArePreserved(graph, pattern, mapped_pattern_var, pattern_var,
                                           mapped_graph_vertex, graph_vertex) &&
               AllPatternEdgesArePreserved(graph, pattern, pattern_var, mapped_pattern_var,
                                           graph_vertex, mapped_graph_vertex);
    });
}

}  // namespace algos::detail
