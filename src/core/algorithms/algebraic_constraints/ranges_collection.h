#pragma once

#include <vector>

#include "numeric_type.h"
#include "type_wrapper.h"
#include "typed_column_pair.h"

namespace algos {

/* A set of ranges for a specific pair of columns */
struct RangesCollection {
    RangesCollection(model::TypeId id, std::vector<std::byte const*>&& ranges, size_t lhs_i,
                     size_t rhs_i)
        : col_pair{{lhs_i, rhs_i}, TypeWrapper(id)}, ranges(std::move(ranges)) {}

    TypedColumnPair col_pair;
    /* Border values of the intervals. Even element --
     * left border. Odd -- right */
    std::vector<std::byte const*> ranges;
};

}  // namespace algos
