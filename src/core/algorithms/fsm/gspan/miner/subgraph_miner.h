#pragma once

#include "types/dfscode.h"
#include "types/frequent_subgraph.h"
#include "types/graph.h"
#include "types/history.h"
#include "types/min_graph.h"
#include "types/projection.h"
#include "utils/thread_pool.h"

namespace gspan {

class SubgraphMiner {
    std::vector<csr_graph_t> const& graph_database_;
    std::vector<FrequentSubgraph> frequent_subgraphs_;

    History history_;
    std::vector<int> rightmost_path_;
    MinGraph min_graph_;
    MinProjection min_projection_;

    size_t min_sup_;
    size_t max_number_of_edges_;
    ThreadPool* thread_pool_ = nullptr;
    std::vector<std::unique_ptr<SubgraphMiner>>* miners_ = nullptr;
    int thread_id_ = 0;

    void MineChild(Projection const& projection, ExtendedEdge const& new_edge, DFSCode code);
    void MineSubgraph(Projection const& projection, DFSCode const& code);

    void Enumerate(DFSCode const& code, Projection const& projection,
                   ProjectionMapBackward& backward_pmap, ProjectionMapForward& forward_pmap);
    void GetBackward(ProjectionEntry const& entry, csr_graph_t const& graph, DFSCode const& code,
                     ProjectionMapBackward& backward_pmap);
    void GetFirstForward(ProjectionEntry const& entry, csr_graph_t const& graph,
                         DFSCode const& code, ProjectionMapForward& forward_pmap);
    void GetOtherForward(ProjectionEntry const& entry, csr_graph_t const& graph,
                         DFSCode const& code, ProjectionMapForward& forward_pmap);

    bool IsCanonical(DFSCode const& code);
    bool IsProjectionMin(DFSCode const& code);
    bool IsBackwardMin(DFSCode const& code, ExtendedEdge const& ee, size_t projection_start_index);
    bool IsForwardMin(DFSCode const& code, ExtendedEdge const& ee, size_t projection_start_index);
    bool ExistsBackwards(size_t projection_start_index);

    void UpdateRightmostPath(DFSCode const& code, size_t size);

public:
    void SetParallelContext(ThreadPool* pool, std::vector<std::unique_ptr<SubgraphMiner>>* miners,
                            int thread_id) {
        thread_pool_ = pool;
        miners_ = miners;
        thread_id_ = thread_id;
    }

    SubgraphMiner(std::vector<csr_graph_t> const& graph_database, int min_sup,
                  int max_number_of_edges)
        : graph_database_(graph_database),
          min_sup_(min_sup),
          max_number_of_edges_(max_number_of_edges) {
        int max_edges = 0;
        int max_vertices = 0;
        for (size_t i = 0; i < graph_database_.size(); i++) {
            auto& graph = graph_database_[i];
            max_edges = std::max(max_edges, static_cast<int>(boost::num_edges(graph)));
            max_vertices = std::max(max_vertices, static_cast<int>(boost::num_vertices(graph)));
        }

        history_.Reset(max_edges, max_vertices);
    }

    void MineFromSeed(Projection const& projection, ExtendedEdge const& seed) {
        DFSCode code;
        MineChild(projection, seed, code);
    }

    std::vector<FrequentSubgraph>& GetFrequentSubgraphs() {
        return frequent_subgraphs_;
    }
};
};  // namespace gspan