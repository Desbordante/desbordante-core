#pragma once

#include "algorithms/mde/hymde/record_match_indexes/orders/non_negative_float.h"

namespace algos::hymde::record_match_indexes::orders {
// An order on doubles in the [0.0, 1.0] range.
// In particular, its least and greatest elements must be in that range.
class Similarity : public NonNegativeFloat {
public:
    static bool IsValid(Type const& value) {
        return 0.0 <= value && value <= 1.0;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
