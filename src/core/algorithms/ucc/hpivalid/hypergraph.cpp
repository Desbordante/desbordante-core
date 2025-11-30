#include "core/algorithms/ucc/hpivalid/hypergraph.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos::hpiv {

Hypergraph::Hypergraph(Edge::size_type num_vertices) : num_vertices_(num_vertices), edges_() {}

void Hypergraph::AddEdgeAndMinimizeInclusion(Edge const &new_edge) {
    // is new_edge a supset of an edge in edges_?
    bool is_supset = false;
    // list of indices of supsets of new_edge from edges_ in descending order
    std::vector<std::vector<Edge>::size_type> supsets_indices;
    for (std::vector<Edge>::size_type i_e = this->NumEdges(); i_e-- > 0;) {
        if (edges_[i_e].is_subset_of(new_edge)) {
            is_supset = true;
            break;
        }

        if (new_edge.is_subset_of(edges_[i_e])) {
            supsets_indices.push_back(i_e);
        }
    }

    if (!is_supset) {
        for (std::vector<Edge>::size_type i_e : supsets_indices) {
            edges_[i_e] = edges_[edges_.size() - 1];
            edges_.pop_back();
        }
        edges_.push_back(new_edge);
    }
}

}  // namespace algos::hpiv
