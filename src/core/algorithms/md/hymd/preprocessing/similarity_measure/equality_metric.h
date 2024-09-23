#pragma once

namespace algos::hymd::preprocessing::similarity_measure {
template <typename T>
double EqualityMetric(T const& obj1, T const& obj2) {
    return obj1 == obj2 ? 1.0 : 0.0;
}
}  // namespace algos::hymd::preprocessing::similarity_measure
