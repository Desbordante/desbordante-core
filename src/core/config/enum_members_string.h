#pragma once

#include <algorithm>
#include <array>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

#include "core/util/enum_to_str.h"

namespace util {
template <typename EnumType>
inline constexpr auto kEnumValuesCStrBuffer = []() constexpr {
    // example: [name1|name2|name3]\0

    constexpr auto&& enum_values = magic_enum::enum_values<EnumType>(); /* C++23: |
                                   std::views::transform([](auto&& v) { return EnumToStr(v); });*/

    constexpr std::size_t enum_count = enum_values.size();
    constexpr std::size_t total_length = []() {
        constexpr std::size_t separator_count = enum_count - 1;
        constexpr std::size_t brackets_count = 2;
        std::size_t names_len = 0;
        for (auto member : enum_values) names_len += EnumToStr(member).size();
        return brackets_count + names_len + separator_count;
    }();

    std::array<char, total_length> arr;

    auto it = arr.begin();
    *it++ = '[';
    static_assert(!enum_values.empty());
    it = std::ranges::copy(EnumToStr(enum_values.front()), it).out;
    for (auto member : enum_values | std::views::drop(1)) {
        *it++ = '|';
        it = std::ranges::copy(EnumToStr(member), it).out;
    }
    *it++ = ']';

    return arr;
}();
}  // namespace util
