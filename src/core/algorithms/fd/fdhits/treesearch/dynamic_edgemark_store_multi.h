#pragma once

#include <vector>

#include "core/algorithms/fd/fdhits/edges/default_edge.h"
#include "core/algorithms/fd/fdhits/edges/multi_fd.h"
#include "core/algorithms/fd/fdhits/treesearch/edgemark.h"
#include "core/algorithms/fd/fdhits/treesearch/hypergraph.h"
#include "core/algorithms/fd/fdhits/treesearch/node_type.h"

namespace algos::fd::fdhits {

class DynamicEdgemarkStoreMulti {
private:
    Hypergraph<edges::MultiFD>* graph_;
    std::vector<Edgemark> vertex_hittings_;
    std::vector<std::vector<Edgemark>> critical_;
    Edgemark uncov_;
    std::vector<NodeType> nodes_;
    std::vector<edges::MultiFD> cand_;
    Edgemark current_vertex_hitting_;

    void UpdateVertexHittings(size_t offset);
    void CalcRhsHitting();
    void CheckCriticalityForActiveRhs();
    void RemoveLhsForSingularRhs();
    bool CanAdd(size_t vertex) const;
    bool CanAddRhs(size_t vertex) const;
    edges::MultiFD& CopyLastCand();

    static void AddCritLevel(std::vector<std::vector<Edgemark>>& critical, Edgemark const& current,
                             Edgemark& uncov, size_t pos);

public:
    explicit DynamicEdgemarkStoreMulti(Hypergraph<edges::MultiFD>* graph);

    std::vector<NodeType> const& GetNodes() const {
        return nodes_;
    }

    std::vector<edges::MultiFD> const& GetCand() const {
        return cand_;
    }

    edges::MultiFD const& GetCurrentCand() const {
        return cand_[nodes_.size()];
    }

    edges::MultiFD& GetCurrentCandMut() {
        return cand_[nodes_.size()];
    }

    bool IsFullyCovered() const {
        return uncov_.IsEmpty();
    }

    std::vector<Edgemark> const& GetCurrentCriticals() const {
        return critical_[nodes_.size()];
    }

    class UncoveredEdgesIterator {
    private:
        Hypergraph<edges::MultiFD> const* graph_;
        Edgemark::OnesIterator it_;
        Edgemark::OnesIterator end_;

    public:
        UncoveredEdgesIterator(Hypergraph<edges::MultiFD> const* g, Edgemark::OnesIterator it,
                               Edgemark::OnesIterator end)
            : graph_(g), it_(it), end_(end) {}

        edges::MultiFD const& operator*() const {
            return graph_->GetEdge(*it_);
        }

        edges::MultiFD const* operator->() const {
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

    bool AddLhs(size_t lhs);
    bool AddRhs(edges::DefaultEdge const& rhs);
    void Pop();
    void Extend(edges::MultiFD const& s, Hypergraph<edges::MultiFD>&& new_edges);
};

}  // namespace algos::fd::fdhits
