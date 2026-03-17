#include "core/algorithms/fd/fdhits/treesearch/dynamic_edgemark_store.h"

#include <cassert>

namespace algos::fd::fdhits {

DynamicEdgemarkStore::DynamicEdgemarkStore(Hypergraph<EdgeType>* graph)
    : graph_(graph), uncov_(Edgemark::Filled(graph->GetEdgeCount())) {
    size_t const vc = graph->GetVertexCount();
    size_t const ec = graph->GetEdgeCount();

    vertex_hittings_.reserve(vc);
    for (size_t i = 0; i < vc; ++i) {
        vertex_hittings_.emplace_back(ec);
    }

    critical_.reserve(vc + 1);
    for (size_t i = 0; i <= vc; ++i) {
        critical_.emplace_back(i, Edgemark::Empty(ec));
    }

    cand_.reserve(vc + 1);
    for (size_t i = 0; i <= vc; ++i) {
        cand_.push_back(EdgeType::Filled(vc));
    }

    nodes_.reserve(vc);

    UpdateVertexHittings(0);
}

void DynamicEdgemarkStore::UpdateVertexHittings(size_t offset) {
    size_t idx = offset;
    for (auto it = graph_->begin() + offset; it != graph_->end(); ++it, ++idx) {
        for (size_t v : it->GetNodes()) {
            assert(v < vertex_hittings_.size());
            vertex_hittings_[v].Set(idx);
        }
    }
}

bool DynamicEdgemarkStore::IsInCriticals(size_t vertex) const {
    auto const& crits = critical_[nodes_.size()];
    auto const& vh = vertex_hittings_[vertex];
    for (auto const& em : crits) {
        if (em.IsSubset(vh)) return true;
    }
    return false;
}

void DynamicEdgemarkStore::AddCritLevelWithHitting(Edgemark const& vertex_hitting) {
    size_t const pos = nodes_.size();
    auto& prev_crits = critical_[pos - 1];
    auto& new_crits = critical_[pos];

    for (size_t i = 0; i < prev_crits.size(); ++i) {
        new_crits[i].CloneFrom(prev_crits[i]);
        new_crits[i] -= vertex_hitting;
    }

    Edgemark& last = new_crits.back();
    last.CloneFrom(vertex_hitting);
    last &= uncov_;
}

void DynamicEdgemarkStore::AddCritLevel(size_t vertex) {
    AddCritLevelWithHitting(vertex_hittings_[vertex]);
}

bool DynamicEdgemarkStore::Add(size_t vertex) {
    if (IsInCriticals(vertex)) return false;

    nodes_.push_back(node_type::LHS(vertex));

    size_t const pos = nodes_.size();
    cand_[pos] = cand_[pos - 1];
    cand_[pos].Remove(vertex);

    AddCritLevel(vertex);

    uncov_ -= vertex_hittings_[vertex];
    return true;
}

void DynamicEdgemarkStore::Pop() {
    if (nodes_.empty()) return;

    auto const removed = nodes_.back();
    nodes_.pop_back();

    if (node_type::IsLHS(removed)) {
        GetCurrentCandMut().Add(node_type::GetLHS(removed));
    }

    uncov_ |= critical_[nodes_.size() + 1].back();
}

void DynamicEdgemarkStore::Extend(EdgeType const& s, Hypergraph<EdgeType>&& new_edges) {
    if (new_edges.GetEdgeCount() == 0) return;

    size_t const previous_size = graph_->GetEdgeCount();

    graph_->Reserve(previous_size + new_edges.GetEdgeCount());
    for (auto& e : new_edges) {
        if (!graph_->Implies(e)) {
            graph_->AddEdge(std::move(e));
        }
    }

    if (previous_size == graph_->GetEdgeCount()) return;

    for (auto& vh : vertex_hittings_) {
        vh.Grow(graph_->GetEdgeCount());
    }
    UpdateVertexHittings(previous_size);

    uncov_.Grow(graph_->GetEdgeCount());

    for (auto& crits : critical_) {
        for (auto& crit : crits) {
            crit.Grow(graph_->GetEdgeCount());
        }
    }

    size_t idx = previous_size;
    for (auto it = graph_->begin() + previous_size; it != graph_->end(); ++it, ++idx) {
        if (!s.Covers(*it)) {
            uncov_.Set(idx);
        } else {
            std::optional<size_t> first;
            std::optional<size_t> end_node;

            for (size_t i = 0; i < nodes_.size(); ++i) {
                if (node_type::IsLHS(nodes_[i]) && it->Contains(node_type::GetLHS(nodes_[i]))) {
                    if (!first.has_value()) first = i;
                    end_node = i + 1;
                }
            }

            if (first.has_value()) {
                size_t end_val = end_node.value_or(nodes_.size());
                for (size_t i = first.value(); i < end_val; ++i) {
                    critical_[i + 1][first.value()].Set(idx);
                }
            }
        }
    }
}

}  // namespace algos::fd::fdhits
