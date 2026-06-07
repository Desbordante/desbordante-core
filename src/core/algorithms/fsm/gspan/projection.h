#pragma once

#include <vector>

#include "extended_edge.h"
#include "graph.h"

namespace gspan {

// Represents an embedding of subgraph edge in real graph
struct ProjectionEntry {
    size_t graph_id;
    vertex_t source;
    edge_t edge;
    ProjectionEntry const* prev;
};

// A projection is a collection of entries across multiple graphs
using Projection = std::vector<ProjectionEntry>;

struct MinProjectionEntry {
    vertex_t source;
    edge_t edge;
    int prev;
};

// Used for minimality check in single graph
using MinProjection = std::vector<MinProjectionEntry>;

using ProjectionMap = std::map<ExtendedEdge, Projection, ExtendedEdgeProjectCompare>;
using ProjectionMapBackward = std::map<ExtendedEdge, Projection, ExtendedEdgeBackwardCompare>;
using ProjectionMapForward = std::map<ExtendedEdge, Projection, ExtendedEdgeForwardCompare>;

}  // namespace gspan
