#pragma once

#include <limits>

#include "algorithms/mde/hymde/record_match_indexes/orders/greater_eq.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/unsigned_integer.h"

namespace algos::hymde::record_match_indexes::orders {
class UnsignedIntegerGe : public GreaterEq<UnsignedInteger> {
public:
    Type LeastElement() const final {
        return std::numeric_limits<Type>::max();
    }

    Type GreatestElement() const final {
        return std::numeric_limits<Type>::min();
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
