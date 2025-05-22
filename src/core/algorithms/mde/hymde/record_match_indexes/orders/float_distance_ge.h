#pragma once

#include <limits>

#include "algorithms/mde/hymde/record_match_indexes/orders/greater_eq.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/non_negative_float.h"

namespace algos::hymde::record_match_indexes::orders {
class FloatDistanceGe : public GreaterEq<NonNegativeFloat> {
public:
    Type LeastElement() const final {
        return std::numeric_limits<Type>::infinity();
    }

    Type GreatestElement() const final {
        return 0.0;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
