#pragma once

#include "algorithms/mde/hymde/record_match_indexes/orders/greater_eq.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/similarity.h"

namespace algos::hymde::record_match_indexes::orders {
class SimilarityGe final : public GreaterEq<Similarity> {
public:
    Type LeastElement() const final {
        return 1.0;
    }

    Type GreatestElement() const final {
        return 0.0;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
