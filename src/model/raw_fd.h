#pragma once

#include <cstddef>
#include <utility>

#include <boost/dynamic_bitset.hpp>

struct RawFD {
    boost::dynamic_bitset<> lhs_;
    size_t rhs_;

    RawFD(boost::dynamic_bitset<> lhs, size_t rhs) noexcept : lhs_(std::move(lhs)), rhs_(rhs) {}
};
