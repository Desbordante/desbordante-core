#pragma once

#include <algorithm>
#include <vector>

#include <boost/unordered_set.hpp>

#include "core/algorithms/fd/fdhits/edges/default_edge.h"
#include "core/algorithms/fd/fdhits/edges/multi_fd.h"
#include "core/algorithms/fd/fdhits/treesearch/hypergraph.h"

namespace algos::fd::fdhits {

template <typename E>
class EdgeHashCollector {
private:
    boost::unordered_set<E, typename E::Hash> edges_;

public:
    void Consume(E const& edge) {
        edges_.insert(edge);
    }

    auto const& GetEdgeSet() const {
        return edges_;
    }

    bool IsEmpty() const {
        return edges_.empty();
    }

    Hypergraph<E> ToGraph(E const& s) {
        Hypergraph<E> graph(s.Size());

        std::vector<E> sorted_edges;
        sorted_edges.reserve(edges_.size());
        for (auto const& edge : edges_) {
            sorted_edges.push_back(edge);
        }
        edges_.clear();

        std::sort(sorted_edges.begin(), sorted_edges.end());

        for (auto const& edge : sorted_edges) {
            if (!graph.Implies(edge)) {
                graph.AddEdge(edge);
            }
        }
        return graph;
    }
};

}  // namespace algos::fd::fdhits
