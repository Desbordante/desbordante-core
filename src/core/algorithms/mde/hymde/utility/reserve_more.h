#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace algos::hymde::utility {
template <typename T>
void ReserveMore(std::vector<T>& vec, std::size_t const size) {
    std::size_t const old_capacity = vec.capacity();
    if (old_capacity >= size) return;
    vec.reserve(std::max(old_capacity * 2, size));
}
}  // namespace algos::hymde::utility
