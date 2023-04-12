#pragma once

#include <vector>

#include "algorithms/algebraic_constraints/ranges_collection.h"
#include "model/column_layout_typed_relation_data.h"

namespace algos {

class ACAlgorithm;

class ACExceptionFinder {
public:
    /* Row that has value pairs, that are exceptions */
    struct ACException {
        ACException(size_t row_i, std::pair<size_t, size_t> col_pair)
            : row_i(row_i), column_pairs{col_pair} {}
        ACException(size_t row_i, std::vector<std::pair<size_t, size_t>> column_pairs)
            : row_i(row_i), column_pairs(std::move(column_pairs)) {}
        /* Row index */
        size_t row_i;
        /* Column pairs, where exception was found in this row */
        std::vector<std::pair<size_t, size_t>> column_pairs;
    };

private:
    std::vector<ACException> exceptions_;
    ACAlgorithm const* ac_alg_;

    /* Creates new ACException and adds it to exceptions_ or adds col_pair to existing one */
    void AddException(size_t row_i, std::pair<size_t, size_t> const& col_pair);
    void CollectColumnPairExceptions(std::vector<model::TypedColumnData> const& data,
                                     RangesCollection const& ranges_collection);

public:
    void CollectExceptions(ACAlgorithm const* ac_alg);
    std::vector<ACException> const& GetACExceptions() const {
        return exceptions_;
    }
};

}  // namespace algos
