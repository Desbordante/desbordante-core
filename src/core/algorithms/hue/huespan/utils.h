#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace algos::huespan::util {
std::vector<std::string> SplitString(std::string str, char delimeter) {
    std::stringstream ss(str);
    std::string token;
    std::vector<std::string> tokens;
    while (getline(ss, token, delimeter)) {
        tokens.push_back(token);
    }
    return tokens;
}
}  // namespace algos::huespan::util