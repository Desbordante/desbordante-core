#pragma once

#include <optional>
#include <vector>

#include "core/algorithms/fd/fdhits/edges/default_edge.h"
#include "core/algorithms/fd/fdhits/treesearch/edgemark.h"
#include "core/algorithms/fd/fdhits/treesearch/hypergraph.h"
#include "core/algorithms/fd/fdhits/treesearch/node_type.h"

namespace algos::fd::fdhits {

class DynamicEdgemarkStore {
protected:
    Hypergraph<edges::DefaultEdge>* graph_;
    std::vector<Edgemark> vertex_hittings_;
    std::vector<std::vector<Edgemark>> critical_;
    Edgemark uncov_;
    std::vector<NodeType> nodes_;
    std::vector<edges::DefaultEdge> cand_;

    void UpdateVertexHittings(size_t offset);
    void AddCritLevel(size_t vertex);
    bool IsInCriticals(size_t vertex) const;
    void AddCritLevelWithHitting(Edgemark const& vertex_hitting);

public:
    using EdgeType = edges::DefaultEdge;

    explicit DynamicEdgemarkStore(Hypergraph<EdgeType>* graph);

    bool IsFullyCovered() const {
        return uncov_.IsEmpty();
    }

    std::vector<NodeType> const& GetNodes() const {
        return nodes_;
    }

    std::vector<EdgeType> const& GetCand() const {
        return cand_;
    }

    EdgeType const& GetCurrentCand() const {
        return cand_[nodes_.size()];
    }

    EdgeType& GetCurrentCandMut() {
        return cand_[nodes_.size()];
    }

    std::vector<Edgemark> const& CurrentCriticals() const {
        return critical_[nodes_.size()];
    }

    class UncoveredEdgesIterator {
    private:
        Hypergraph<EdgeType> const* graph_;
        Edgemark::OnesIterator it_;
        Edgemark::OnesIterator end_;

    public:
        UncoveredEdgesIterator(Hypergraph<EdgeType> const* graph, Edgemark::OnesIterator it,
                               Edgemark::OnesIterator end)
            : graph_(graph), it_(it), end_(end) {}

        EdgeType const& operator*() const {
            return graph_->GetEdge(*it_);
        }

        EdgeType const* operator->() const {
            return &graph_->GetEdge(*it_);
        }

        UncoveredEdgesIterator& operator++() {
            ++it_;
            return *this;
        }

        bool operator!=(UncoveredEdgesIterator const& other) const {
            return it_ != other.it_;
        }
    };

    UncoveredEdgesIterator UncoveredEdgesBegin() const {
        return UncoveredEdgesIterator(graph_, uncov_.OnesBegin(), uncov_.OnesEnd());
    }

    UncoveredEdgesIterator UncoveredEdgesEnd() const {
        return UncoveredEdgesIterator(graph_, uncov_.OnesEnd(), uncov_.OnesEnd());
    }

    bool Add(size_t vertex);
    void Pop();
    void Extend(EdgeType const& s, Hypergraph<EdgeType>&& new_edges);
};

}  // namespace algos::fd::fdhits
