#pragma once

#include <cmath>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/config/names_and_descriptions.h"
#include "frequent_subgraph.h"
#include "graph.h"

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
    std::vector<gspan::graph_t> graph_database_;

    void FindAllOnlyOneVertex();
    void RemoveInfrequentLabel(gspan::graph_t& graph, int label);
    void RemoveInfrequentVertexPairs();
    void GSpanDFS(gspan::DFSCode const& code, std::unordered_set<int> graph_ids);

    std::unordered_map<gspan::ExtendedEdge, std::unordered_set<int>, gspan::ExtendedEdge::Hash>
    RightMostPathExtensions(gspan::DFSCode const& code, std::unordered_set<int> graph_ids);

    std::unordered_set<gspan::ExtendedEdge, gspan::ExtendedEdge::Hash>
    RightMostPathExtensionsFromSingle(gspan::DFSCode const& code, gspan::graph_t const& graph);

    bool IsCanonical(gspan::DFSCode const& code);

    unsigned long long ExecuteInternal();

    void ResetState();
    void LoadDataInternal();

    void RegisterOptions();

public:
    void MineSubgraphs();

    GSpan();

    std::vector<gspan::FrequentSubgraph> const& GetFrequentSubgraphs() const {
        return frequent_subgraphs_;
    }

    std::vector<gspan::graph_t> const& GetGraphDatabase() const {
        return graph_database_;
    }

    int GetMinSup() const {
        return min_sup_;
    }
};
}  // namespace algos