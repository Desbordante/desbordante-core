#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/util/bitset_concept.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class MMCS {
private:
    using Edge = Bitset;
    using Edgemark = boost::dynamic_bitset<>;

    std::vector<Edge> hypergraph_;
    std::size_t num_vertices_;

    std::vector<Edge> column_to_dif_funcs_;

    void UpdateCritAndUncov(std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                            std::vector<Edgemark>& crit, Edgemark& uncov,
                            Edgemark const& v_hittings) const {
        // update crit[] for vertices in S and put changes on stack

        removed_criticals_stack.push_back(crit);

        std::ranges::for_each(removed_criticals_stack.back(),
                              [&v_hittings](Edgemark& edgemark) { edgemark -= v_hittings; });
        std::ranges::for_each(crit, [&v_hittings](Edgemark& edgemark) { edgemark &= v_hittings; });

        // set critical edges for v and remove them from uncov

        crit.emplace_back(uncov - v_hittings);
        uncov &= v_hittings;
    }

    void RestoreCritAndUncov(std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                             std::vector<Edgemark>& crit, Edgemark& uncov) const {
        uncov |= crit.back();
        crit.pop_back();

        for (std::size_t i = 0; i < crit.size(); ++i) {
            crit[i] |= removed_criticals_stack.back()[i];
        }

        removed_criticals_stack.pop_back();
    }

    bool VertexWouldViolate(std::vector<Edgemark> const& crit, Edgemark const& v_hittings) const {
        for (Edgemark const& em : crit) {
            if (!em.intersects(v_hittings)) {
                return true;
            }
        }

        return false;
    }

    void ExtendOrConfirmS(Edge& s, Edge& cand, std::vector<Edgemark>& crit, Edgemark& uncov,
                          std::vector<Edgemark>& vertex_hittings,
                          std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                          std::vector<Edge>& result) const {
        // find edge from uncov with smallest intersecton C with CAND
        Edge c = cand;
        c -= hypergraph_[uncov.find_first()];
        for (std::size_t i_e = uncov.find_next(uncov.find_first()); i_e != Edgemark::npos;
             i_e = uncov.find_next(i_e)) {
            Edge c_new = cand;
            c_new -= hypergraph_[i_e];
            if (c_new.count() < c.count()) {
                c = std::move(c_new);
            }
        }

        cand -= c;

        for (std::size_t i = 0; i != column_to_dif_funcs_.size(); ++i) {
            Edge cur_edge_column_intersect = s;
            cur_edge_column_intersect &= column_to_dif_funcs_[i];
            // add only one vertex per column
            if (cur_edge_column_intersect.none()) {
                Edge column_candidates = c;
                column_candidates &= column_to_dif_funcs_[i];
                for (std::size_t v = column_candidates.find_first(); v != Edge::npos;
                     v = column_candidates.find_next(v)) {
                    // don't branch if v is violater for S
                    if (VertexWouldViolate(crit, vertex_hittings[v])) {
                        continue;
                    }

                    // branch
                    UpdateCritAndUncov(removed_criticals_stack, crit, uncov, vertex_hittings[v]);

                    s.set(v);
                    if (uncov.none()) {
                        result.push_back(s);
                    } else if (!cand.none()) {
                        ExtendOrConfirmS(s, cand, crit, uncov, vertex_hittings,
                                         removed_criticals_stack, result);
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

public:
    MMCS(std::vector<Bitset> hypergraph, std::vector<Bitset> const& column_to_dif_funcs,
         std::size_t cur_column_index)
        : hypergraph_(std::move(hypergraph)),
          num_vertices_(hypergraph_[0].size()),
          column_to_dif_funcs_() {
        column_to_dif_funcs_.reserve(column_to_dif_funcs.size() - 1);
        for (std::size_t i = 0; i != column_to_dif_funcs.size(); ++i) {
            if (i == cur_column_index) {
                continue;
            }
            column_to_dif_funcs_.push_back(column_to_dif_funcs[i]);
        }
    }

    std::vector<Edge> GetCovers() const {
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
        for (std::size_t i_e = 0; i_e < hypergraph_.size(); ++i_e) {
            for (std::size_t i_v = hypergraph_[i_e].find_first(); i_v != Edge::npos;
                 i_v = hypergraph_[i_e].find_next(i_v)) {
                vertex_hittings[i_v].set(i_e);
            }
        }

        std::vector<std::vector<Edgemark>> removed_criticals_stack;
        std::vector<Edge> result;

        ExtendOrConfirmS(s, cand, crit, uncov, vertex_hittings, removed_criticals_stack, result);

        return result;
    }
};

}  // namespace algos::dd
