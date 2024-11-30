#pragma once

#include <cstring>
#include <type_traits>

namespace algos::hymd::utility {

// https://en.cppreference.com/w/cpp/numeric/bit_cast
template <class To, class From>
std::enable_if_t<sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From> &&
                         std::is_trivially_copyable_v<To>,
                 To>
// constexpr support needs compiler magic
BitCast(From const& src) noexcept {
    static_assert(std::is_trivially_constructible_v<To>,
                  "This implementation additionally requires destination type to be trivially "
                  "constructible");

    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

}  // namespace algos::hymd::utility
