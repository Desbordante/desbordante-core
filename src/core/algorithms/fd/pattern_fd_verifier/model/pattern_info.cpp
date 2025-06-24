#include "pattern_info.h"

#include <regex>

namespace algos::pattern_fd {

std::string RegexPatternInfo::ToRegex(std::string const& pattern_regex) const {
    std::string regex_str;
    size_t i = 0;
    while (i < pattern_regex.size()) {
        if (pattern_regex[i] == '\\' && i + 1 < pattern_regex.size()) {
            switch (pattern_regex[i + 1]) {
                case 'A':
                    regex_str += ".*";
                    break;
                case 'D':
                    regex_str += "\\d";
                    break;
                case 'U':
                    regex_str += "[A-Z]";
                    break;
                case 'L':
                    regex_str += "[a-z]";
                    break;
                case 'S':
                    regex_str += "[^\\w]";
                    break;
                default:
                    throw std::invalid_argument("Invalid escape sequence: \\" +
                                                std::string(1, pattern_regex[i + 1]));
            }
            i += 2;
        } else if (pattern_regex[i] == '{') {
            size_t end = pattern_regex.find('}', i);
            if (end == std::string::npos) {
                throw std::invalid_argument("Unclosed quantifier at position " + std::to_string(i));
            }
            std::string count_str = pattern_regex.substr(i + 1, end - i - 1);
            regex_str += "{" + count_str + "}";
            i = end + 1;
        } else {
            regex_str += std::string(1, pattern_regex[i]);
            i++;
        }
    }
    return regex_str;
}

}  // namespace algos::pattern_fd