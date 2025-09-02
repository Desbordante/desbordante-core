#include "pattern_info.h"

#include <regex>

namespace algos::pattern_fd {

std::string RegexPatternInfo::ToRegex(std::string const& pattern_regex) {
    std::string regex_str;
    size_t i = 0;

    while (i < pattern_regex.size()) {
        if (pattern_regex[i] == '<') {
            regex_str += '(';
            i++;
            continue;
        }
        if (pattern_regex[i] == '>') {
            regex_str += ')';
            i++;
            continue;
        }

        if (pattern_regex[i] == '\\' && i + 1 < pattern_regex.size()) {
            switch (pattern_regex[i + 1]) {
                case 'A':
                    regex_str += ".";
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

void RegexPatternInfo::CountConstrainedGroups(std::string const& pattern_regex) {
    size_t i = 0;
    while (i < pattern_regex.size()) {
        if (pattern_regex[i] == '<') {
            this->num_constrained_groups_++;
            i++;
        } else {
            i++;
        }
    }
}

std::vector<std::string> RegexPatternInfo::ExtractConstrainedParts(std::string const& value) const {
    std::vector<std::string> parts;
    if (num_constrained_groups_ == 0) {
        return parts;
    }

    std::smatch match_results;
    std::regex re(this->regex_);

    if (std::regex_match(value, match_results, re)) {
        for (size_t group_index = 1; group_index <= num_constrained_groups_; ++group_index) {
            if (group_index < match_results.size()) {
                parts.push_back(match_results[group_index].str());
            } else {
                break;
            }
        }
    }
    return parts;
}

void RegexPatternInfo::ExtractLiterals(std::string const& p) {
    std::string current;
    size_t current_pos_in_regex = 0;

    for (size_t i = 0; i < p.size(); ++i) {
        char c = p[i];
        if (c == '\\') {
            if (!current.empty()) {
                literals_.emplace_back(current, current_pos_in_regex - current.size());
                current.clear();
            }
            if (i + 1 < p.size()) {
                char next = p[i + 1];
                switch (next) {
                    case 'A':
                        current_pos_in_regex += 1;
                        break;
                    case 'D':
                        current_pos_in_regex += 1;
                        break;
                    case 'U':
                        current_pos_in_regex += 1;
                        break;
                    case 'L':
                        current_pos_in_regex += 1;
                        break;
                    case 'S':
                        current_pos_in_regex += 1;
                        break;
                    default:
                        break;
                }
                i++;
            }
        } else if (c == '{') {
            size_t end_brace = p.find('}', i);
            if (end_brace != std::string::npos) {
                std::string num_str = p.substr(i + 1, end_brace - i - 1);
                int count = std::stoi(num_str);
                current_pos_in_regex += count;
                i = end_brace;
            }
        } else if (c == '.' || c == '^' || c == '$' || c == '|' || c == '?' || c == '+' ||
                   c == '*' || c == '[' || c == ']' || c == '(' || c == ')' || c == '<' ||
                   c == '>') {
            if (!current.empty()) {
                literals_.emplace_back(current, current_pos_in_regex - current.size());
                current.clear();
            }
            current_pos_in_regex += 1;
        } else {
            current += c;
            current_pos_in_regex += 1;
        }
    }

    if (!current.empty()) {
        literals_.emplace_back(current, current_pos_in_regex - current.size());
    }
}

}  // namespace algos::pattern_fd