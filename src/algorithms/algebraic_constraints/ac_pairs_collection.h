#pragma once

#include <memory>
#include <vector>

#include "algorithms/algebraic_constraints/ac.h"
#include "algorithms/algebraic_constraints/typed_column_pair.h"

namespace algos {

using ACPairs = std::vector<std::unique_ptr<ACPair>>;

/* Contains value pairs for a specific pair of columns */
struct ACPairsCollection {
    ACPairsCollection(std::unique_ptr<model::INumericType> num_type, ACPairs&& ac_pairs,
                      size_t lhs_i, size_t rhs_i)
        : col_pair{{lhs_i, rhs_i}, std::move(num_type)}, ac_pairs(std::move(ac_pairs)) {}
    /* Column pair indices and pointer to their type */
    TypedColumnPair col_pair;
    /* Vector with ACPairs */
    ACPairs ac_pairs;
};

}  // namespace algos
