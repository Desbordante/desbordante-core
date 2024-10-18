#pragma once

#include <cstddef>
#include <vector>

namespace util {
template <typename T>
std::vector<T> GetPreallocatedVector(std::size_t const capacity) {
    std::vector<T> vec;
    vec.reserve(capacity);
    return vec;
}
}  // namespace util
