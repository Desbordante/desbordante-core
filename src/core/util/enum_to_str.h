#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <ranges>
#include <string>

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_switch.hpp>

namespace util {

inline std::string SnakeToKCamel(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 1);
    out.push_back('k');

    bool cap_next = true;
    for (char c : input) {
        if (c == '_') {
            cap_next = true;
        } else if (cap_next) {
            out.push_back(std::toupper(static_cast<unsigned char>(c)));
            cap_next = false;
        } else {
            out.push_back(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return out;
}

template <auto Value>
    requires std::is_enum_v<decltype(Value)>
constexpr auto kSnakeCaseEnumNameCStrChars = []() constexpr {
    constexpr auto orig_name = magic_enum::enum_name(Value);
    // std::isupper and std::tolower are not available at compile time
    constexpr auto isupper = [](char c) constexpr { return 'A' <= c && c <= 'Z'; };
    // C++23: make static and reuse isupper
    constexpr auto tolower = [](char c) constexpr {
        return ('A' <= c && c <= 'Z') ? 'a' + (c - 'A') : c;
    };
    static_assert(orig_name.size() > 1 && orig_name.front() == 'k' && isupper(orig_name[1]));
    constexpr std::size_t name_len = [&]() {
        constexpr std::size_t letters = orig_name.size() - 1;
        constexpr std::size_t underscores =
                std::ranges::count_if(orig_name | std::views::drop(2), isupper);
        return letters + underscores;
    }();
    std::array<char, name_len + 1> name_arr;
    name_arr[name_len] = '\0';

    auto conv_it = name_arr.begin();
    auto orig_it = std::next(orig_name.begin());
    *conv_it++ = tolower(static_cast<unsigned char>(*orig_it++));
    for (; orig_it != orig_name.end(); ++orig_it) {
        if (isupper(*orig_it)) {
            *conv_it++ = '_';
            *conv_it++ = tolower(static_cast<unsigned char>(*orig_it));
        } else {
            *conv_it++ = *orig_it;
        }
    }

    return name_arr;
}();

template <typename E>
    requires std::is_enum_v<E>
constexpr std::string_view EnumToStr(E value) {
    return magic_enum::enum_switch(
            [](auto value) {
                constexpr E v = value;
                constexpr auto& c_str_chars = kSnakeCaseEnumNameCStrChars<v>;
                return std::string_view{c_str_chars.data(), c_str_chars.size() - 1};
            },
            value);
}

template <typename E>
    requires std::is_enum_v<E>
std::optional<E> EnumFromStr(std::string_view str) {
    return magic_enum::enum_cast<E>(SnakeToKCamel(str));
}

}  // namespace util
