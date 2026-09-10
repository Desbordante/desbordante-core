#pragma once

#include <vector>

#include <boost/unordered/unordered_flat_set.hpp>

#include "types/graph.h"

namespace gspan {

template <typename GraphType>
boost::unordered_flat_set<int> TranslateToOriginalIds(
        boost::unordered_flat_set<int> const& internal_ids,
        std::vector<GraphType> const& graph_db) {
    boost::unordered_flat_set<int> original_ids;
    original_ids.reserve(internal_ids.size());
    for (int id : internal_ids) {
        original_ids.insert(graph_db[id][boost::graph_bundle].original_id);
    }
    return original_ids;
}

inline csr_graph_t ConvertToCSR(graph_t const& src_graph) {
    using EdgePair = std::pair<int, int>;
    std::vector<EdgePair> edges;
    std::vector<Edge> edge_props;

    edges.reserve(boost::num_edges(src_graph));
    edge_props.reserve(boost::num_edges(src_graph));

    auto edge_iters = boost::edges(src_graph);
    for (auto it = edge_iters.first; it != edge_iters.second; ++it) {
        int u = boost::source(*it, src_graph);
        int v = boost::target(*it, src_graph);

        edges.emplace_back(u, v);
        edge_props.push_back(src_graph[*it]);
    }

    csr_graph_t csr(boost::edges_are_unsorted_multi_pass, edges.begin(), edges.end(),
                    edge_props.begin(), boost::num_vertices(src_graph),
                    src_graph[boost::graph_bundle]);

    auto vertex_iters = boost::vertices(src_graph);
    for (auto it = vertex_iters.first; it != vertex_iters.second; ++it) {
        csr[*it] = src_graph[*it];
    }

    return csr;
}

}  // namespace gspan