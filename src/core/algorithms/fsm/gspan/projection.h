#pragma once

#include <deque>
#include <span>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

#include "extended_edge.h"
#include "graph.h"

namespace gspan {

struct HistoryNode {
    HistoryNode const* prev;
    vertex_t added_vertex;
};

struct ProjectionEntry {
    int graph_id;
    std::vector<HistoryNode const*> history_leaves;
};

// A projection is a collection of entries across multiple graphs
using Projection = std::vector<ProjectionEntry>;

using projection_map_t = boost::unordered_flat_map<ExtendedEdge, Projection, ExtendedEdge::Hash>;

}  // namespace gspan
