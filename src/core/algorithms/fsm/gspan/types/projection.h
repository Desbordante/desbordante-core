#pragma once

#include <vector>

#include <boost/container/flat_map.hpp>

#include "extended_edge.h"
#include "graph.h"

namespace gspan {

// Represents an embedding of subgraph edge in real graph
struct ProjectionEntry {
    int graph_id;
    csr_edge_t edge;
    ProjectionEntry const* prev;
};

// A projection is a collection of entries across multiple graphs
class Projection {
    std::vector<ProjectionEntry> entries_;
    size_t support_ = 0;
    int prev_graph_id_ = -1;

public:
    void PushBack(int graph_id, csr_edge_t edge, ProjectionEntry const* prev) {
        if (graph_id != prev_graph_id_) {
            support_++;
            prev_graph_id_ = graph_id;
        }
        entries_.push_back({graph_id, edge, prev});
    }

    auto begin() const noexcept {
        return entries_.begin();
    }

    auto end() const noexcept {
        return entries_.end();
    }

    size_t GetSupport() const noexcept {
        return support_;
    }
};

struct MinEdge;

struct MinProjectionEntry {
    MinEdge const* edge;
    int prev;
};

// Used for minimality check in single graph
using MinProjection = std::vector<MinProjectionEntry>;

using ProjectionMap =
        boost::container::flat_map<ExtendedEdge, Projection, ExtendedEdgeProjectCompare>;
using ProjectionMapBackward =
        boost::container::flat_map<ExtendedEdge, Projection, ExtendedEdgeBackwardCompare>;
using ProjectionMapForward =
        boost::container::flat_map<ExtendedEdge, Projection, ExtendedEdgeForwardCompare>;

}  // namespace gspan
