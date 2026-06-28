#include "history.h"

namespace gspan {

void History::Reconstruct(ProjectionEntry const& start, csr_graph_t const& graph) {
    if (current_ == nullptr || current_graph_ != &graph) {
        // No current encoding, encode from scratch
        std::fill_n(edge_visited_.begin(), boost::num_edges(graph), false);
        std::fill_n(vertex_counts_.begin(), boost::num_vertices(graph), 0);
        edge_size_ = 0;

        ProjectionEntry const* current_entry = &start;
        do {
            auto edge = current_entry->edge;

            edges_[edge_size_++] = edge;
            edge_visited_[graph[edge].id] = true;
            vertex_counts_[graph[boost::source(edge, graph)].id]++;
            vertex_counts_[graph[boost::target(edge, graph)].id]++;
        } while ((current_entry = current_entry->prev) != nullptr);

        current_graph_ = &graph;
    } else {
        // Has an encoded projection, assume it is the same size and distinct.
        // (allows the do-while)
        // Reuse as much of this as possible.

        ProjectionEntry const* new_entry = &start;
        ProjectionEntry const* old_entry = current_;

        size_t modify_index = 0;

        do {
            edges_[modify_index++] = new_entry->edge;

            // Remove old edge
            auto old_edge = old_entry->edge;
            edge_visited_[graph[old_edge].id].flip();
            vertex_counts_[graph[boost::source(old_edge, graph)].id]--;
            vertex_counts_[graph[boost::target(old_edge, graph)].id]--;

            // Add new edge
            auto new_edge = new_entry->edge;
            edge_visited_[graph[new_edge].id].flip();
            vertex_counts_[graph[boost::source(new_edge, graph)].id]++;
            vertex_counts_[graph[boost::target(new_edge, graph)].id]++;
        } while ((new_entry = new_entry->prev) != (old_entry = old_entry->prev));
    }
    current_ = &start;
}

void History::ReconstructEdges(MinProjection const& projection, csr_graph_t const& graph,
                               int start) {
    std::fill_n(edge_visited_.begin(), boost::num_edges(graph), false);
    edge_size_ = 0;

    do {
        auto const& current_entry = projection[start];
        auto edge = current_entry.edge;
        edges_[edge_size_++] = edge;
        edge_visited_[graph[edge].id] = 1;
        start = current_entry.prev;
    } while (start != -1);
}

void History::ReconstructVertices(MinProjection const& projection, csr_graph_t const& graph,
                                  int start) {
    std::fill_n(vertex_counts_.begin(), boost::num_vertices(graph), 0);
    edge_size_ = 0;

    do {
        auto const& current_entry = projection[start];
        auto edge = current_entry.edge;
        edges_[edge_size_++] = edge;
        vertex_counts_[graph[boost::source(edge, graph)].id] = 1;
        vertex_counts_[graph[boost::target(edge, graph)].id] = 1;
        start = current_entry.prev;
    } while (start != -1);
}

}  // namespace gspan