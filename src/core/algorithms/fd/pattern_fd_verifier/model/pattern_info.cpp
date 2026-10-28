#include "core/algorithms/fd/pattern_fd_verifier/model/pattern_info.h"

#include <charconv>
#include <stdexcept>
#include <string_view>

namespace {

constexpr bool IsSpecialRegexChar(char c) {
    return c == '.' || c == '^' || c == '$' || c == '|' || c == '?' || c == '+' || c == '*' ||
           c == '[' || c == ']' || c == '(' || c == ')';
}

}  // namespace

namespace algos::pattern_fd {

RegexPatternInfo::RegexPatternInfo(std::string const& pattern) {
    regex_.reserve(pattern.size() * 2);
    size_t group_count = 0;

    std::string_view p(pattern);
    size_t i = 0;

    while (i < p.size()) {
        char c = p[i];

        if (c == '<') {
            regex_ += '(';
            constrained_groups_.push_back(++group_count);
            i++;
        } else if (c == '>') {
            regex_ += ')';
            i++;
        } else if (c == '\\') {
            if (i + 1 >= p.size()) {
                throw std::invalid_argument("Invalid escape sequence at end of pattern");
            }
            char next = p[i + 1];
            switch (next) {
                case 'A':
                    regex_ += ".";
                    break;
                case 'D':
                    regex_ += "\\d";
                    break;
                case 'U':
                    regex_ += "[A-Z]";
                    break;
                case 'L':
                    regex_ += "[a-z]";
                    break;
                case 'S':
                    regex_ += "[^\\w]";
                    break;
                default:
                    throw std::invalid_argument(std::string("Invalid escape sequence: \\") + next);
            }
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
            i = end + 1;
        } else if (IsSpecialRegexChar(c)) {
            if (c == '(' && (i + 1 == p.size() || p[i + 1] != '?')) ++group_count;
            regex_ += c;
            i++;
        } else {
            regex_ += c;
            i++;
        }
    }

    compiled_regex_.assign(regex_, std::regex::optimize);
}

bool RegexPatternInfo::Matches(std::string const& value) const {
    return std::regex_match(value, compiled_regex_);
}

std::optional<std::vector<std::string>> RegexPatternInfo::MatchAndExtract(
        std::string const& value) const {
    std::smatch match_results;
    if (!std::regex_match(value, match_results, compiled_regex_)) return std::nullopt;

    std::vector<std::string> parts;
    parts.reserve(constrained_groups_.size());
    for (size_t group_index : constrained_groups_) {
        parts.push_back(match_results[group_index].str());
    }
    return parts;
}

std::vector<std::string> RegexPatternInfo::ExtractConstrainedParts(std::string const& value) const {
    return MatchAndExtract(value).value_or(std::vector<std::string>{});
}

}  // namespace algos::pattern_fd
