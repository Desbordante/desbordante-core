#pragma once

#include <cstddef>
#include <ranges>
#include <vector>

namespace algos::hymd::utility {
inline auto IndexRange(std::size_t n) {
    return std::views::iota(std::size_t(0), n);
}
}  // namespace algos::hymd::utility
