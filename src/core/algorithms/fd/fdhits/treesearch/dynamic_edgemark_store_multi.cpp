#include "core/algorithms/fd/fdhits/treesearch/dynamic_edgemark_store_multi.h"

#include <optional>

namespace algos::fd::fdhits {

DynamicEdgemarkStoreMulti::DynamicEdgemarkStoreMulti(Hypergraph<edges::MultiFD>* graph)
    : graph_(graph),
      uncov_(Edgemark::Filled(graph->GetEdgeCount())),
      current_vertex_hitting_(graph->GetEdgeCount()) {
    size_t const vertex_count = graph->GetVertexCount();
    size_t const edge_count = graph->GetEdgeCount();

    critical_.reserve(vertex_count + 1);
    for (size_t i = 0; i <= vertex_count; ++i) {
        critical_.emplace_back(i, Edgemark::Empty(edge_count));
    }

    vertex_hittings_.resize(vertex_count / 2, Edgemark::Empty(edge_count));

    cand_.resize(vertex_count + 1, edges::MultiFD::Filled(vertex_count));

    nodes_.reserve(vertex_count);

    UpdateVertexHittings(0);
}

bool DynamicEdgemarkStoreMulti::CanAdd(size_t vertex) const {
    auto const& current_crits = GetCurrentCriticals();

    for (size_t i = 0; i < current_crits.size(); ++i) {
        if (node_type::IsLHS(nodes_[i])) {
            if (current_crits[i].IsSubset(vertex_hittings_[vertex])) {
                return false;
            }
        }
    }
    return true;
}

bool DynamicEdgemarkStoreMulti::CanAddRhs(size_t vertex) const {
    auto const& current_crits = GetCurrentCriticals();

    for (size_t i = 0; i < current_crits.size(); ++i) {
        if (node_type::IsLHS(nodes_[i])) {
            if (IsSubsetInverse(current_crits[i], vertex_hittings_[vertex])) {
                return false;
            }
        }
    }
    return true;
}

edges::MultiFD& DynamicEdgemarkStoreMulti::CopyLastCand() {
    size_t const pos = nodes_.size();
    cand_[pos + 1].SetTo(cand_[pos]);
    return cand_[pos + 1];
}

void DynamicEdgemarkStoreMulti::CalcRhsHitting() {
    auto const& current_rhs = cand_[nodes_.size()].GetRhs();
    auto rhs_nodes = current_rhs.GetNodes();

    if (rhs_nodes.empty()) {
        return;
    }

    current_vertex_hitting_.SetToNot(vertex_hittings_[rhs_nodes[0]]);

    for (size_t i = 1; i < rhs_nodes.size(); ++i) {
        size_t col = rhs_nodes[i];
        current_vertex_hitting_.IntersectWithInverse(vertex_hittings_[col]);
    }
}

void DynamicEdgemarkStoreMulti::CheckCriticalityForActiveRhs() {
    auto rhs_nodes = GetCurrentCand().GetRhsNodes();

    for (size_t rhs : rhs_nodes) {
        if (!CanAddRhs(rhs)) {
            GetCurrentCandMut().RemoveRhs(rhs);
        }
    }
}

void DynamicEdgemarkStoreMulti::RemoveLhsForSingularRhs() {
    if (GetCurrentCand().RhsCount() == 1) {
        size_t rhs = GetCurrentCand().GetRhsNodes()[0];
        GetCurrentCandMut().RemoveLhs(rhs);
    }
}

void DynamicEdgemarkStoreMulti::AddCritLevel(std::vector<std::vector<Edgemark>>& critical,
                                             Edgemark const& current, Edgemark& uncov, size_t pos) {
    auto& prev_layer = critical[pos - 1];
    auto& curr_layer = critical[pos];

    for (size_t i = 0; i < prev_layer.size(); ++i) {
        curr_layer[i].SetTo(prev_layer[i]);
        curr_layer[i] -= current;
    }

    Edgemark& last = curr_layer.back();

    last.SetTo(current);

    last &= uncov;
    uncov -= current;
}

bool DynamicEdgemarkStoreMulti::AddLhs(size_t lhs) {
    if (!CanAdd(lhs)) {
        return false;
    }

    auto& cand = CopyLastCand();

    cand.Remove(lhs);
    cand.RemoveRhs(lhs);

    if (cand.RhsCount() == 0) {
        return false;
    }

    nodes_.push_back(node_type::LHS(lhs));

    AddCritLevel(critical_, vertex_hittings_[lhs], uncov_, nodes_.size());

    CheckCriticalityForActiveRhs();

    RemoveLhsForSingularRhs();

    return true;
}

bool DynamicEdgemarkStoreMulti::AddRhs(edges::DefaultEdge const& rhs) {
    auto& cand = CopyLastCand();
    cand.IntersectRhs(rhs);

    nodes_.push_back(node_type::RHS());
    CalcRhsHitting();

    AddCritLevel(critical_, current_vertex_hitting_, uncov_, nodes_.size());

    RemoveLhsForSingularRhs();

    return true;
}

void DynamicEdgemarkStoreMulti::Pop() {
    if (nodes_.empty()) return;

    auto removed = nodes_.back();
    nodes_.pop_back();

    if (node_type::IsLHS(removed)) {
        size_t lhs = node_type::GetLHS(removed);
        GetCurrentCandMut().Add(lhs);
    }

    uncov_ |= critical_[nodes_.size() + 1].back();
}

void DynamicEdgemarkStoreMulti::UpdateVertexHittings(size_t offset) {
    for (size_t idx = offset; idx < graph_->GetEdgeCount(); ++idx) {
        auto const& edge = graph_->GetEdge(idx);
        for (size_t v : edge.GetLhs().GetNodes()) {
            vertex_hittings_[v].Set(idx);
        }
    }
}

void DynamicEdgemarkStoreMulti::Extend(edges::MultiFD const& s,
                                       Hypergraph<edges::MultiFD>&& new_edges) {
    size_t previous_size = graph_->GetEdgeCount();

    graph_->Reserve(previous_size + new_edges.GetEdgeCount());
    for (auto&& e : new_edges) {
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

    for (size_t idx = previous_size; idx < graph_->GetEdgeCount(); ++idx) {
        auto const& edge = graph_->GetEdge(idx);

        if (!s.Covers(edge)) {
            uncov_.Set(idx);
        } else {
            std::optional<size_t> first;
            std::optional<size_t> end;

            for (size_t i = 0; i < nodes_.size(); ++i) {
                bool covers = false;

                auto const& next_cand = cand_[i + 1];

                if (auto* lhs_ptr = std::get_if<size_t>(&nodes_[i])) {
                    if (edge.Contains(*lhs_ptr) || next_cand.GetRhs().Implies(edge.GetRhs())) {
                        covers = true;
                    }
                } else {
                    if (next_cand.GetRhs().Implies(edge.GetRhs())) {
                        covers = true;
                    }
                }

                if (covers) {
                    if (!first.has_value()) {
                        first = i;
                    }
                } else if (first.has_value()) {
                    end = i;
                    break;
                }
            }

            if (first.has_value()) {
                size_t end_val = end.value_or(nodes_.size());
                for (size_t i = first.value(); i < end_val; ++i) {
                    critical_[i + 1][first.value()].Set(idx);
                }
            }
        }
    }
}

}  // namespace algos::fd::fdhits
