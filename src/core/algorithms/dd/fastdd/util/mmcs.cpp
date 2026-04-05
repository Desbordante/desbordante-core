#include "core/algorithms/dd/fastdd/util/mmcs.h"

namespace algos::dd {

std::vector<boost::dynamic_bitset<>> MMCS::GetCovers() const {
    // S, CAND
    Edge s(num_vertices_);
    Edge cand(num_vertices_);
    cand.set();

    // crit, uncov
    std::vector<Edgemark> crit;
    Edgemark uncov(hypergraph_.size());
    uncov.set();

    // vertex hittings
    std::vector<Edgemark> vertex_hittings(num_vertices_, Edgemark(hypergraph_.size()));
    for (std::vector<Edge>::size_type i_e = 0; i_e < hypergraph_.size(); ++i_e) {
        for (Edge::size_type i_v = hypergraph_[i_e].find_first(); i_v != Edge::npos;
             i_v = hypergraph_[i_e].find_next(i_v)) {
            vertex_hittings[i_v].set(i_e);
        }
    }

    std::vector<std::vector<Edgemark>> removed_criticals_stack;
    std::vector<boost::dynamic_bitset<>> result;

    ExtendOrConfirmS(s, cand, crit, uncov, vertex_hittings, removed_criticals_stack, result);

    return result;
}

void MMCS::UpdateCritAndUncov(std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                              std::vector<Edgemark>& crit, Edgemark& uncov,
                              Edgemark const& v_hittings) const {
    // update crit[] for vertices in S and put changes on stack

    removed_criticals_stack.emplace_back(crit.size());

    for (std::vector<Edgemark>::size_type i = 0; i < crit.size(); ++i) {
        removed_criticals_stack.back()[i] = crit[i] & v_hittings;
        crit[i] -= v_hittings;
    }

    // set critical edges for v and remove them from uncov

    crit.emplace_back(v_hittings & uncov);
    uncov -= v_hittings;
}

void MMCS::RestoreCritAndUncov(std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                               std::vector<Edgemark>& crit, Edgemark& uncov) const {
    uncov |= crit.back();
    crit.pop_back();

    for (std::vector<Edgemark>::size_type i = 0; i < crit.size(); ++i) {
        crit[i] |= removed_criticals_stack.back()[i];
    }

    removed_criticals_stack.pop_back();
}

bool MMCS::VertexWouldViolate(std::vector<Edgemark> const& crit, Edgemark const& v_hittings) const {
    for (Edgemark const& em : crit) {
        if (em.is_subset_of(v_hittings)) {
            return true;
        }
    }

    return false;
}

void MMCS::ExtendOrConfirmS(Edge& s, Edge& cand, std::vector<Edgemark>& crit, Edgemark& uncov,
                            std::vector<Edgemark>& vertex_hittings,
                            std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                            std::vector<boost::dynamic_bitset<>>& result) const {
    // find edge from uncov with smallest intersecton C with CAND
    Edge c = hypergraph_[uncov.find_first()] & cand;
    for (Edge::size_type i_e = uncov.find_next(uncov.find_first()); i_e != Edge::npos;
         i_e = uncov.find_next(i_e)) {
        Edge c_new = (hypergraph_[i_e] & cand);
        if (c_new.count() < c.count()) {
            c = std::move(c_new);
        }
    }

    cand -= c;

    for (std::size_t i = 0; i != column_to_dif_funcs_.size(); ++i) {
        Edge cur_edge_column_intersect = s & column_to_dif_funcs_[i];
        // add only one vertex per column
        if (cur_edge_column_intersect.none()) {
            Edge column_candidates = c & column_to_dif_funcs_[i];
            for (Edge::size_type v = column_candidates.find_first(); v != Edge::npos;
                 v = column_candidates.find_next(v)) {
                // don't branch if v is violater for S
                if (VertexWouldViolate(crit, vertex_hittings[v] - uncov)) {
                    continue;
                }

                // branch
                UpdateCritAndUncov(removed_criticals_stack, crit, uncov, vertex_hittings[v]);

                s.set(v);
                if (uncov.none()) {
                    result.push_back(s);
                } else if (!cand.none()) {
                    ExtendOrConfirmS(s, cand, crit, uncov, vertex_hittings, removed_criticals_stack,
                                     result);
                }
                // update CAND
                cand.set(v);
                s.reset(v);
                RestoreCritAndUncov(removed_criticals_stack, crit, uncov);
            }
        }
    }

    // add C now to CAND; this also adds the violaters to CAND again and it is
    // equal to the CAND passed in
    cand |= c;
}

}  // namespace algos::dd
