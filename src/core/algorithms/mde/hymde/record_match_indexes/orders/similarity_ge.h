#pragma once

#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"

namespace algos::hymde::record_match_indexes::orders {
class SimilarityGe final : public TotalOrder<double> {
public:
    using Type = double;

    std::string ToString() const final {
        return ">=";
    }

    bool AreInOrder(Type const& res1, Type const& res2) const final {
        return res1 >= res2;
    }

    Type LeastElement() const final {
        return 1.0;
    }

    Type GreatestElement() const final {
        return 0.0;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
