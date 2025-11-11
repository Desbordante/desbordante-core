#pragma once

#include <string>

namespace python_bindings {
inline std::string SnakeToKCamel(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 1);
    out.push_back('k');

    bool capitalize_next = true;

    for (char c : input) {
        if (c == '_') {
            capitalize_next = true;
        } else {
            if (capitalize_next) {
                out.push_back(std::toupper(static_cast<unsigned char>(c)));
                capitalize_next = false;
            } else {
                out.push_back(std::tolower(static_cast<unsigned char>(c)));
            }
        }
    }
    return out;
}

inline std::string KCamelToSnake(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    size_t start_idx = (input.size() > 1 && input[0] == 'k' && std::isupper(input[1])) ? 1 : 0;

    for (size_t i = start_idx; i < input.size(); ++i) {
        char c = input[i];
        if (std::isupper(c)) {
            if (i > start_idx) {
                out.push_back('_');
            }
            out.push_back(std::tolower(static_cast<unsigned char>(c)));
        } else {
            out.push_back(c);
        }
    }
    return out;
}
}  // namespace python_bindings
