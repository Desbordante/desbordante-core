#pragma once

#include <algorithm>
#include <cstddef>
#include <unordered_set>

namespace util {
template <typename T>
std::size_t IntersectionSize(std::unordered_set<T> const& set1, std::unordered_set<T> const& set2) {
    return std::count_if(set1.begin(), set1.end(),
                         [&](T const& element) { return set2.contains(element); });
}
}  // namespace util
