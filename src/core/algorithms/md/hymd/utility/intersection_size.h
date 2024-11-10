#pragma once

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <unordered_set>

namespace algos::hymd::utility {
template <typename T>
std::size_t IntersectionSize(std::unordered_set<T> const& set1, std::unordered_set<T> const& set2) {
    return std::ranges::count_if(set1, [&](T const& element) { return set2.contains(element); });
}
}  // namespace algos::hymd::utility
