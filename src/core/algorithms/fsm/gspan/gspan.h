#pragma once

#include <cmath>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "core/algorithms/algorithm.h"
#include "core/config/names_and_descriptions.h"
#include "frequent_subgraph.h"
#include "graph.h"
#include "history.h"
#include "projection.h"

namespace algos {
class GSpan : public Algorithm {
protected:
    // The minimum support represented as a count (number of subgraph occurrences)
    int min_sup_;

    // The minimum support represented as a frequency (a value between 0 and 1)
    double min_frequency_;

    // The vector of frequent subgraphs found by the last execution
    std::vector<gspan::FrequentSubgraph> frequent_subgraphs_;

    std::vector<int> frequent_vertex_labels_;

    bool output_single_vertices_;

    // Maximum number of edges in each frequent subgraph
    int max_number_of_edges_ = INT_MAX;

    // Empty graphs removed count
    int empty_graphs_removed_;

    std::filesystem::path graph_database_path_;
    std::filesystem::path output_path_;
    std::vector<gspan::graph_t> raw_dataset_;
    std::vector<gspan::graph_t> pruned_graphs_;

    gspan::History history_;
    gspan::graph_t min_graph_;
    gspan::MinProjection min_projection_;

    void FindAllOnlyOneVertex();
    void RemoveInfrequentLabel(gspan::graph_t& graph, int label);
    void RemoveInfrequentVertexPairs();
    void CompactIds();

    gspan::ProjectionMap GetInitialEdges();
    void MineChild(gspan::Projection const& projection, gspan::ExtendedEdge const& new_edge,
                   gspan::DFSCode const& code);
    void MineSubgraph(gspan::Projection const& projection, gspan::DFSCode& code);

    void GSpanDFS(gspan::DFSCode const& code, gspan::Projection const& embeddings);

    void Enumerate(gspan::DFSCode const& code, gspan::Projection const& projection,
                   gspan::ProjectionMapBackward& backward_pmap,
                   gspan::ProjectionMapForward& forward_pmap);

    gspan::ProjectionMap RightMostPathExtensions(gspan::DFSCode const& code,
                                                 gspan::Projection const& embeddings);

    boost::unordered_flat_map<gspan::ExtendedEdge, gspan::MinProjection, gspan::ExtendedEdge::Hash>
    RightMostPathExtensionsFromSingle(gspan::DFSCode const& code, gspan::graph_t const& graph);

    bool IsCanonical(gspan::DFSCode const& code);
    bool IsProjectionMin(gspan::DFSCode const& code);
    bool IsBackwardMin(gspan::DFSCode const& code, gspan::ExtendedEdge const& ee,
                       size_t projection_start_index);
    bool IsForwardMin(gspan::DFSCode const& code, gspan::ExtendedEdge const& ee,
                      size_t projection_start_index);
    bool ExistsBackwards(gspan::DFSCode const& code, size_t projection_start_index);

    unsigned long long ExecuteInternal();

    void ResetState();
    void LoadDataInternal();
    void MakeExecuteOptsAvailable();

    void RegisterOptions();

public:
    void Launch();

    GSpan();

    std::vector<gspan::FrequentSubgraph> const& GetFrequentSubgraphs() const {
        return frequent_subgraphs_;
    }

    int GetMinSup() const {
        return min_sup_;
    }
};
}  // namespace algos