#pragma once

#include <limits>

#include "algorithms/mde/hymde/record_match_indexes/orders/less_eq.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/valid_float.h"

namespace algos::hymde::record_match_indexes::orders {
class FloatLe : public LessEq<ValidFloat> {
public:
    Type LeastElement() const final {
        return -std::numeric_limits<Type>::infinity();
    }

    Type GreatestElement() const final {
        return std::numeric_limits<Type>::infinity();
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
