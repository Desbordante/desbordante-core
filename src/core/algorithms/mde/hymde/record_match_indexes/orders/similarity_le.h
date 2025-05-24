#pragma once

#include "algorithms/mde/hymde/record_match_indexes/orders/similarity.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/less_eq.h"

namespace algos::hymde::record_match_indexes::orders {
class SimilarityLe final : public LessEq<Similarity> {
public:
    Type LeastElement() const final {
        return 0.0;
    }

    Type GreatestElement() const final {
        return 1.0;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
