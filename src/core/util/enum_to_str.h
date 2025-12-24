#pragma once

#include <cctype>
#include <optional>
#include <string>

#include <magic_enum/magic_enum.hpp>

namespace util {

inline std::string KCamelToSnake(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    size_t start = (input.size() > 1 && input[0] == 'k' && std::isupper(input[1])) ? 1 : 0;

    for (size_t i = start; i < input.size(); ++i) {
        char c = input[i];
        if (std::isupper(c)) {
            if (i > start) out.push_back('_');
            out.push_back(std::tolower(static_cast<unsigned char>(c)));
        } else {
            out.push_back(c);
        }
    }
    return out;
}

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

template <typename E>
    requires std::is_enum_v<E>
std::string EnumToStr(E value) {
    return KCamelToSnake(magic_enum::enum_name(value));
}

template <typename E>
    requires std::is_enum_v<E>
std::optional<E> EnumFromStr(std::string_view str) {
    return magic_enum::enum_cast<E>(SnakeToKCamel(str));
}

}  // namespace util
