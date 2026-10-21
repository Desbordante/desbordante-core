#include "utils.h"

namespace gspan {

boost::unordered_flat_set<int> TranslateToOriginalIds(
        boost::unordered_flat_set<int> const& internal_ids, std::vector<graph_t> const& graph_db) {
    boost::unordered_flat_set<int> original_ids;
    original_ids.reserve(internal_ids.size());
    for (int const id : internal_ids) {
        original_ids.insert(graph_db[id][boost::graph_bundle].original_id);
    }
    return original_ids;
}

csr_graph_t ConvertToCSR(graph_t const& src_graph) {
    using EdgePair = std::pair<int, int>;
    std::vector<EdgePair> edges;
    std::vector<Edge> edge_props;

    edges.reserve(boost::num_edges(src_graph));
    edge_props.reserve(boost::num_edges(src_graph));

    for (auto const edge : boost::make_iterator_range(boost::edges(src_graph))) {
        edges.emplace_back(boost::source(edge, src_graph), boost::target(edge, src_graph));
        edge_props.push_back(src_graph[edge]);
    }

    csr_graph_t csr(boost::edges_are_unsorted_multi_pass, edges.begin(), edges.end(),
                    edge_props.begin(), boost::num_vertices(src_graph),
                    src_graph[boost::graph_bundle]);

    for (auto const vertex : boost::make_iterator_range(boost::vertices(src_graph))) {
        csr[vertex] = src_graph[vertex];
    }

    return csr;
}

}  // namespace gspan