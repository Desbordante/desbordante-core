#pragma once

#include <boost/dynamic_bitset.hpp>

namespace algos::cfdfinder {
struct Candidate {
    boost::dynamic_bitset<> lhs_;
    size_t rhs_;

    Candidate() = default;

    Candidate(boost::dynamic_bitset<> lhs, size_t rhs) noexcept : lhs_(std::move(lhs)), rhs_(rhs) {}

    bool operator==(Candidate const& other) const {
        return rhs_ == other.rhs_ && lhs_ == other.lhs_;
    }
};
}  // namespace algos::cfdfinder

template <>
struct std::hash<algos::cfdfinder::Candidate> {
    size_t operator()(algos::cfdfinder::Candidate const& candidate) const {
        size_t h1 = std::hash<boost::dynamic_bitset<>>{}(candidate.lhs_);
        size_t h2 = std::hash<int>{}(candidate.rhs_);
        return h1 ^ (h2 << 1);
    }
};