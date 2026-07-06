#include "pattern_info.h"

#include <charconv>

namespace {

constexpr bool IsSpecialRegexChar(char c) {
    return c == '.' || c == '^' || c == '$' || c == '|' || c == '?' || c == '+' ||
           c == '*' || c == '[' || c == ']' || c == '(' || c == ')';
}

}  // namespace

namespace algos::pattern_fd {

RegexPatternInfo::RegexPatternInfo(std::string const& pattern) {
    regex_.reserve(pattern.size() * 2);
    literals_.reserve(pattern.size() / 2);

    std::string current_literal;
    size_t pos = 0;

    auto flush_literal = [&]() {
        if (!current_literal.empty()) {
            literals_.emplace_back(current_literal, pos - current_literal.size());
            current_literal.clear();
        }
    };

    std::string_view p(pattern);
    size_t i = 0;

    while (i < p.size()) {
        char c = p[i];

        if (c == '<') {
            flush_literal();
            regex_ += '(';
            num_constrained_groups_++;
            pos += 1;
            i++;
        } else if (c == '>') {
            flush_literal();
            regex_ += ')';
            pos += 1;
            i++;
        } else if (c == '\\') {
            flush_literal();
            if (i + 1 >= p.size()) {
                throw std::invalid_argument("Invalid escape sequence at end of pattern");
            }
            char next = p[i + 1];
            switch (next) {
                case 'A': regex_ += ".";      break;
                case 'D': regex_ += "\\d";    break;
                case 'U': regex_ += "[A-Z]";  break;
                case 'L': regex_ += "[a-z]";  break;
                case 'S': regex_ += "[^\\w]"; break;
                default:
                    throw std::invalid_argument(std::string("Invalid escape sequence: \\") + next);
            }
            pos += 1;
            i += 2;
        } else if (c == '{') {
            size_t end = p.find('}', i);
            if (end == std::string_view::npos) {
                throw std::invalid_argument("Unclosed quantifier at position " + std::to_string(i));
            }
            size_t count = 0;
            auto result = std::from_chars(p.data() + i + 1, p.data() + end, count);
            if (result.ec != std::errc()) {
                throw std::invalid_argument("Invalid number in quantifier");
            }
            regex_ += '{';
            regex_.append(p.substr(i + 1, end - i - 1));
            regex_ += '}';
            pos += count;
            i = end + 1;
        } else if (IsSpecialRegexChar(c)) {
            flush_literal();
            regex_ += c;
            pos += 1;
            i++;
        } else {
            current_literal += c;
            regex_ += c;
            pos += 1;
            i++;
        }
    }

    flush_literal();
    compiled_regex_.assign(regex_, std::regex::optimize);
}

std::vector<std::string> RegexPatternInfo::ExtractConstrainedParts(std::string const& value) const {
    std::vector<std::string> parts;
    if (num_constrained_groups_ == 0) {
        return parts;
    }

    std::smatch match_results;

    if (std::regex_match(value, match_results, compiled_regex_)) {
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

}  // namespace algos::pattern_fd
