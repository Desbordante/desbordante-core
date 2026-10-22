#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace algos::huespan::util {
inline std::vector<std::string> SplitString(std::string str, char delimiter) {
    std::stringstream ss(str);
    std::string token;
    std::vector<std::string> tokens;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
}  // namespace algos::huespan::util