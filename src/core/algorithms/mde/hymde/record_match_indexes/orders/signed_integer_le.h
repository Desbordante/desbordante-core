#pragma once

#include <limits>

#include "algorithms/mde/hymde/record_match_indexes/orders/less_eq.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/signed_integer.h"

namespace algos::hymde::record_match_indexes::orders {
class SignedIntegerLe : public LessEq<SignedInteger> {
public:
    Type LeastElement() const final {
        return std::numeric_limits<Type>::min();
    }

    Type GreatestElement() const final {
        return std::numeric_limits<Type>::max();
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
