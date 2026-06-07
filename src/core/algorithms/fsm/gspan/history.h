#pragma once

#include <tuple>

#include "graph.h"
#include "projection.h"

namespace gspan {

class History {
    std::vector<std::pair<vertex_t, edge_t>> edges_;
    std::vector<bool> edge_visited_;
    std::vector<int> vertex_counts_;
    size_t edge_size_;

    ProjectionEntry const* current_ = nullptr;
    graph_t const* current_graph_ = nullptr;

public:
    History() = default;

    History(int max_edges, int max_vertices)
        : edges_(max_edges),
          edge_visited_(max_edges, 0),
          vertex_counts_(max_vertices, 0),
          edge_size_(0) {}

    void Reset(int max_edges, int max_vertices) {
        edges_.resize(max_edges);
        edge_visited_.assign(max_edges, false);
        vertex_counts_.assign(max_vertices, 0);
        edge_size_ = 0;
        current_ = nullptr;
        current_graph_ = nullptr;
    }

    void Reconstruct(ProjectionEntry const& start, graph_t const& graph);

    void ReconstructEdges(MinProjection const& projection, graph_t const& graph, int start);

    void ReconstructVertices(MinProjection const& projection, graph_t const& graph, int start);

    bool HasVertex(size_t index) const {
        return vertex_counts_[index] != 0;
    }

    bool HasEdge(size_t index) const {
        return edge_visited_[index];
    }

    std::pair<vertex_t, edge_t> GetEdge(size_t index) const {
        return edges_[edge_size_ - index - 1];
    }

    void Clear() {
        current_ = nullptr;
    }
};

}  // namespace gspan