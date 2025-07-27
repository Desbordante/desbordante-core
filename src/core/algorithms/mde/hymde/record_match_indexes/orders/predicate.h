#pragma once

#include "algorithms/mde/hymde/record_match_indexes/orders/bool.h"

namespace algos::hymde::record_match_indexes::orders {
class Predicate final : public Bool {
public:
    using Type = bool;

    std::string ToString() const final {
        return "(bool)";
    }

    bool AreInOrder(Type const& res1, Type const& res2) const final {
        return res2 || res1 == res2;
    }

    Type LeastElement() const final {
        return false;
    }

    Type GreatestElement() const final {
        return true;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders

