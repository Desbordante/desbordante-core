#pragma once

#include "core/algorithms/fd/fdhits/edges/default_edge.h"
#include "core/algorithms/fd/fdhits/pli_table.h"
#include "core/algorithms/fd/fdhits/types.h"

namespace algos::fd::fdhits::problems {
class FDProblem {
private:
    size_t column_id_;
    PLITable const* table_;

public:
    using EdgeType = edges::DefaultEdge;

    FDProblem(size_t column_id, PLITable const* table) : column_id_(column_id), table_(table) {}

    template <typename C>
    void GetEdge(RowIndex r1, RowIndex r2, C* consumer) const {
        EdgeType edge = EdgeType::Empty(table_->GetColumnCount());
        for (size_t col = 0; col < table_->GetColumnCount(); ++col) {
            if (table_->IsDifferentOnColumn(r1, r2, col)) {
                edge.Add(col);
            }
        }
        if (edge.Contains(column_id_)) {
            consumer->Consume(edge);
        }
    }

    bool IsValidResult(EdgeType const& result) const {
        return !result.Contains(column_id_);
    }

    bool IsViolatingPair(RowIndex r1, RowIndex r2, EdgeType const&) const {
        return table_->IsDifferentOnColumn(r1, r2, column_id_);
    }

    size_t GetColumnId() const {
        return column_id_;
    }
};

}  // namespace algos::fd::fdhits::problems
