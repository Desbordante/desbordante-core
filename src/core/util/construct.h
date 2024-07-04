#pragma once

#include <utility>

namespace util {
template <typename T, typename... Args>
constexpr T Construct(Args&&... args) {
    return T(std::forward<Args>(args)...);
}
}  // namespace util
