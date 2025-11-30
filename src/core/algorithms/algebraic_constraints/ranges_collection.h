#pragma once

#include <vector>

#include "core/algorithms/algebraic_constraints/typed_column_pair.h"
#include "core/model/types/numeric_type.h"

namespace algos {

/* A set of ranges for a specific pair of columns */
struct RangesCollection {
    RangesCollection(std::unique_ptr<model::INumericType> num_type,
                     std::vector<std::byte const*>&& ranges, size_t lhs_i, size_t rhs_i)
        : col_pair{{lhs_i, rhs_i}, std::move(num_type)}, ranges(std::move(ranges)) {}

    TypedColumnPair col_pair;
    /* Border values of the intervals. Even element --
     * left border. Odd -- right */
    std::vector<std::byte const*> ranges;
};

}  // namespace algos
