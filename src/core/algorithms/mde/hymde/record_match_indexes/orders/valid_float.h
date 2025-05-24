#pragma once

#include <cmath>

#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"

namespace algos::hymde::record_match_indexes::orders {
// A float that is not nan.
class ValidFloat : public TotalOrder<double> {
public:
    static bool IsValid(Type const& value) {
        return !std::isnan(value);
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
