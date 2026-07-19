#pragma once

#include <algorithm>
#include <cctype>
#include <numeric>
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
constexpr auto kSnakeCaseEnumNameCStrChars = []() {
    constexpr auto to_snake_case = []() {
        /* The corresponding std functions are not available at compile time. */
        constexpr auto isupper = [](char c) { return 'A' <= c && c <= 'Z'; };
        constexpr auto tolower = [](char c) { return 'a' + (c - 'A'); };
        constexpr auto raw_enum_name = magic_enum::enum_name(Value);
        static_assert(raw_enum_name.size() > 1 && raw_enum_name.front() == 'k' &&
                      isupper(raw_enum_name[1]));
        std::string s;
        s.push_back(tolower(raw_enum_name[1]));
        for (char c : raw_enum_name | std::views::drop(2)) {
            if (isupper(c)) {
                s.push_back('_');
                s.push_back(tolower(c));
            } else {
                s.push_back(c);
            }
        }
        return s;
    };
    constexpr std::size_t snake_case_name_len = to_snake_case().size();
    std::string const snake_case_name = to_snake_case();
    std::array<char, snake_case_name_len + 1> name_cstring_buffer;
    std::ranges::copy(snake_case_name, name_cstring_buffer.begin());
    name_cstring_buffer[snake_case_name_len] = '\0';
    return name_cstring_buffer;
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
