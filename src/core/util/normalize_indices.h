#pragma once

#include <algorithm>

namespace util {
template <typename IndicesType>
inline static void NormalizeIndices(IndicesType& indices) {
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
}
}  // namespace util
