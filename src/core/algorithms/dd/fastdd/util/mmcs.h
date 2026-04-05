#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

using Edge = boost::dynamic_bitset<>;
using Edgemark = boost::dynamic_bitset<>;

class MMCS {
private:
    std::vector<Edge> hypergraph_;
    std::size_t num_vertices_;

    std::vector<boost::dynamic_bitset<>> column_to_dif_funcs_;

    void UpdateCritAndUncov(std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                            std::vector<Edgemark>& crit, Edgemark& uncov,
                            Edgemark const& v_hittings) const;
    void RestoreCritAndUncov(std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                             std::vector<Edgemark>& crit, Edgemark& uncov) const;
    bool VertexWouldViolate(std::vector<Edgemark> const& crit, Edgemark const& v_hittings) const;
    void ExtendOrConfirmS(Edge& s, Edge& cand, std::vector<Edgemark>& crit, Edgemark& uncov,
                          std::vector<Edgemark>& vertex_hittings,
                          std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                          std::vector<boost::dynamic_bitset<>>& result) const;

public:
    MMCS(std::vector<Edge> hypergraph,
         std::vector<boost::dynamic_bitset<>> const& column_to_dif_funcs,
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

        for (auto& edge : hypergraph_) {
            edge.flip();
        }
    }

    std::vector<boost::dynamic_bitset<>> GetCovers() const;
};

}  // namespace algos::dd
