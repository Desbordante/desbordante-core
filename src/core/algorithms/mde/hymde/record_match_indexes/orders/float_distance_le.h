#pragma once

#include <limits>

#include "algorithms/mde/hymde/record_match_indexes/orders/less_eq.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/non_negative_float.h"

namespace algos::hymde::record_match_indexes::orders {
class FloatDistanceLe : public LessEq<NonNegativeFloat> {
public:
    Type LeastElement() const final {
        return 0.0;
    }

    Type GreatestElement() const final {
        return std::numeric_limits<Type>::infinity();
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
