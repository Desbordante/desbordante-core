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
using Projection = std::vector<ProjectionEntry>;

struct MinProjectionEntry {
    csr_edge_t edge;
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
