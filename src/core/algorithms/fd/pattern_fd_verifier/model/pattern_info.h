#pragma once

#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "config/indices/type.h"

namespace algos::pattern_fd {

using Pos = size_t;  // position of the token in the string

using TokenNGram = std::string;  // a token is represented as a string (n-gram)

using Pattern = std::string;

class PatternInfo {
public:
    virtual ~PatternInfo() = default;

    virtual std::string Type() const {
        return "PatternInfo";
    }
};

class TokenPatternInfo : public PatternInfo {
private:
    TokenNGram token_;
    Pos position_;

public:
    TokenPatternInfo(TokenNGram const& token, Pos position) : token_(token), position_(position) {}

    std::string Type() const override {
        return "TokenPatternInfo";
    }

    bool operator==(TokenPatternInfo const& other) const {
        return token_ == other.token_ && position_ == other.position_;
    }

    TokenNGram const& Token() const {
        return token_;
    }

    Pos Position() const {
        return position_;
    }
};

class RegexPatternInfo : public PatternInfo {
private:
    std::string regex_;

    std::string ToRegex(std::string const& pattern_regex) const;

public:
    RegexPatternInfo(std::string const& pattern) : regex_(ToRegex(pattern)) {}

    std::string const& Regex() const {
        return regex_;
    }

    std::string Type() const override {
        return "RegexPatternInfo";
    }
};

using PatternsTable = std::vector<std::unordered_map<
        config::IndexType, std::shared_ptr<PatternInfo>>>;  // table of patterns for attributes

}  // namespace algos::pattern_fd

namespace std {
template <>
struct hash<algos::pattern_fd::TokenPatternInfo> {
    size_t operator()(algos::pattern_fd::TokenPatternInfo const& t) const noexcept {
        size_t h1 = std::hash<std::string_view>{}(t.Token());
        size_t h2 = std::hash<algos::pattern_fd::Pos>{}(t.Position());
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std