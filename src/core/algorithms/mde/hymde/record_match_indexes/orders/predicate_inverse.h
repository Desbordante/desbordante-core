#pragma once

#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"

namespace algos::hymde::record_match_indexes::orders {
class PredicateInverse final : public TotalOrder<bool> {
public:
    using Type = bool;

    std::string ToString() const final {
        return "(!bool)";
    }

    bool AreInOrder(Type const& res1, Type const& res2) const final {
        return !res2 || res1 == res2;
    }

    Type LeastElement() const final {
        return true;
    }

    Type GreatestElement() const final {
        return false;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
