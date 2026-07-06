#pragma once

#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/functional/hash.hpp>

#include "config/indices/type.h"

namespace algos::pattern_fd {

using Pos = size_t;  // position of the token in the string

using TokenNGram = std::string;  // a token is represented as a string (n-gram)

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
    TokenPatternInfo(TokenNGram token, Pos position) : token_(std::move(token)), position_(position) {}

    std::string Type() const override {
        return "TokenPatternInfo";
    }

    bool operator==(TokenPatternInfo const& other) const {
        return token_ == other.token_ && position_ == other.position_;
    }

    bool operator!=(TokenPatternInfo const& other) const {
        return !(*this == other);
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
    std::regex compiled_regex_;
    std::vector<std::pair<std::string, size_t>> literals_;
    size_t num_constrained_groups_ = 0;

public:
    explicit RegexPatternInfo(std::string const& pattern);

    std::string const& Regex() const {
        return regex_;
    }

    std::string Type() const override {
        return "RegexPatternInfo";
    }

    std::vector<std::pair<std::string, size_t>> const& GetLiterals() const {
        return literals_;
    }

    std::vector<std::string> ExtractConstrainedParts(std::string const& value) const;

    bool HasConstraints() const {
        return num_constrained_groups_ > 0;
    }
};

class WildcardPatternInfo : public PatternInfo {
public:
    WildcardPatternInfo() = default;

    std::string Type() const override {
        return "WildcardPatternInfo";
    }
};

using PatternsTable = std::vector<std::unordered_map<
        config::IndexType, std::shared_ptr<PatternInfo>>>;  // table of patterns for attributes

}  // namespace algos::pattern_fd

template <>
struct std::hash<algos::pattern_fd::TokenPatternInfo> {
    size_t operator()(algos::pattern_fd::TokenPatternInfo const& t) const noexcept {
        size_t seed = 0;
        boost::hash_combine(seed, t.Token());
        boost::hash_combine(seed, t.Position());
        return seed;
    }
};