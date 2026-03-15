#pragma once

#include "core/algorithms/fd/fdhits/edges/multi_fd.h"
#include "core/algorithms/fd/fdhits/pli_table.h"
#include "core/algorithms/fd/fdhits/types.h"

namespace algos::fd::fdhits::problems {

class MultiFDProblem {
private:
    PLITable const* table_;
    bool separate_edges_;

public:
    using EdgeType = edges::MultiFD;

    explicit MultiFDProblem(PLITable const* table, bool separate_edges = false)
        : table_(table), separate_edges_(separate_edges) {}

    template <typename C>
    void GetEdge(RowIndex r1, RowIndex r2, C* consumer) const {
        size_t const num_columns = table_->GetColumnCount();

        if (separate_edges_) {
            for (size_t lhs_col = 0; lhs_col < num_columns; ++lhs_col) {
                if (!table_->IsDifferentOnColumn(r1, r2, lhs_col)) continue;
                for (size_t rhs_col = 0; rhs_col < num_columns; ++rhs_col) {
                    if (lhs_col == rhs_col) continue;
                    if (!table_->IsDifferentOnColumn(r1, r2, rhs_col)) {
                        edges::DefaultEdge lhs = edges::DefaultEdge::Empty(num_columns);
                        lhs.Add(lhs_col);
                        edges::DefaultEdge rhs = edges::DefaultEdge::Empty(num_columns);
                        rhs.Add(rhs_col);
                        consumer->Consume(EdgeType(std::move(lhs), std::move(rhs)));
                    }
                }
            }
        } else {
            edges::DefaultEdge lhs = edges::DefaultEdge::Empty(num_columns);
            edges::DefaultEdge rhs = edges::DefaultEdge::Empty(num_columns);

            for (size_t col = 0; col < num_columns; ++col) {
                if (table_->IsDifferentOnColumn(r1, r2, col)) {
                    lhs.Add(col);
                } else {
                    rhs.Add(col);
                }
            }

            if (lhs.Count() > 0) {
                consumer->Consume(EdgeType(std::move(lhs), std::move(rhs)));
            }
        }
    }

    bool IsValidResult(EdgeType const& result) const {
        return result.RhsCount() > 0;
    }

    bool IsViolatingPair(RowIndex r1, RowIndex r2, EdgeType const& candidate) const {
        for (size_t node : candidate.GetRhs().GetNodes()) {
            if (table_->IsDifferentOnColumn(r1, r2, node)) return true;
        }
        return false;
    }
};

}  // namespace algos::fd::fdhits::problems
