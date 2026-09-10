#pragma once

#include <cmath>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "core/algorithms/algorithm.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/thread_number/type.h"
#include "types/frequent_subgraph.h"
#include "types/graph.h"
#include "types/history.h"
#include "types/projection.h"

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

    std::filesystem::path graph_database_path_;
    std::filesystem::path output_path_;

    config::ThreadNumType threads_num_;

    std::vector<gspan::graph_t> raw_dataset_;
    std::vector<gspan::graph_t> pruned_graphs_;
    std::vector<gspan::csr_graph_t> pruned_csr_graphs_;

    void FindAllOnlyOneVertex();
    void RemoveInfrequentLabel(gspan::graph_t& graph, int label);
    void RemoveInfrequentVertexPairs();
    void CompactIds();

    gspan::ProjectionMap GetInitialEdges();

    void ExecuteInternal();

    void ResetState();
    void LoadDataInternal();
    void MakeExecuteOptsAvailable();

    void RegisterOptions();

public:
    GSpan();

    void Launch();

    std::vector<gspan::FrequentSubgraph> const& GetFrequentSubgraphs() const {
        return frequent_subgraphs_;
    }

    int GetMinSup() const {
        return min_sup_;
    }
};
}  // namespace algos