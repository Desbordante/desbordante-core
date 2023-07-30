#include <stdexcept>
#include <sstream>
#include <utility>

#include "attribute_pair.h"
#include "predicates/SingleAttributePredicate.h"

using namespace algos::fastod;

AttributePair::AttributePair(SingleAttributePredicate left, int right) noexcept : pair_(std::make_pair(left, right)) {
    if(left.GetAttribute() == right) {
        throw std::invalid_argument("Two attributes cannot be the same");
    }
}

SingleAttributePredicate AttributePair::GetLeft() const noexcept {
    return pair_.first;
}

int AttributePair::GetRight() const noexcept {
    return pair_.second;
}

std::string AttributePair::ToString() const {
    std::stringstream ss;
    ss << "{ " << pair_.first.ToString() << ", " << pair_.second + 1 << " }";
    return ss.str();
}

namespace algos::fastod {

bool operator==(AttributePair const& x, AttributePair const& y) {
    return x.pair_ == y.pair_;
}

} // namespace algos::fastod
