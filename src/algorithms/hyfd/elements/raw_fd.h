#pragma once

#include <boost/dynamic_bitset.hpp>

namespace algos::hyfd {

struct RawFD {
    boost::dynamic_bitset<> lhs_;
    size_t rhs_;

    RawFD(boost::dynamic_bitset<> lhs, size_t rhs) noexcept : lhs_(std::move(lhs)), rhs_(rhs) {}
};

}  // namespace algos::hyfd
