#pragma once

#include <vector>

#include "typed_column_pair.h"

namespace algos {

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

}  // namespace algos
