#pragma once

#include <array>
#include <ranges>
#include <string>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

#include "core/util/enum_to_str.h"

namespace config {
template <typename EnumType>
    requires std::is_enum_v<EnumType>
inline constexpr auto kEnumMembersCStrBuffer = []() {
    // example: [name1|name2|name3]\0
    using namespace std::string_literals;

    constexpr auto make_string = []() {
        constexpr auto enum_names =
                std::views::transform(magic_enum::enum_values<EnumType>(), [](EnumType v) {
                    // Workaround for being unable to use std::string(util::EnumToStr(v)) directly
                    // with UBSan + GCC
                    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71962
                    return std::string(util::EnumToStr(v).begin(), util::EnumToStr(v).end());
                });
        static_assert(!magic_enum::enum_values<EnumType>().empty());
        return "["s +
               std::accumulate(
                       std::next(enum_names.begin()), enum_names.end(), enum_names.front(),
                       [&](std::string acc, std::string name) { return acc + "|"s + name; }) +
               std::string("]" /* clang+glibc bug, "]"s doesn't compile */);
    };
    constexpr std::size_t total_length = make_string().size();
    std::string const enum_values_string = make_string();
    std::array<char, total_length + 1> arr;
    std::ranges::copy(enum_values_string, arr.begin());
    arr[total_length] = '\0';
    return arr;
}();
}  // namespace config
