# pragma once

#include "predicates/SingleAttributePredicate.h"

namespace algos::fastod {

class AttributePair {
private:
    std::pair<SingleAttributePredicate, int> const pair_;

public:
    AttributePair(SingleAttributePredicate left, int right) noexcept;

    SingleAttributePredicate GetLeft() const noexcept;
    int GetRight() const noexcept;

    std::string ToString() const;
    friend bool operator==(AttributePair const& x, AttributePair const& y);
};

} // namespace algos::fastod 
