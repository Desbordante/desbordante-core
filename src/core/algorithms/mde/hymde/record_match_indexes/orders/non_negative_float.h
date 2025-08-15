#pragma once

#include "algorithms/mde/hymde/record_match_indexes/orders/valid_float.h"

namespace algos::hymde::record_match_indexes::orders {
// A float that is >= 0.
class NonNegativeFloat : public ValidFloat {
public:
    static bool IsValid(Type const& value) {
        return value >= 0.0;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
