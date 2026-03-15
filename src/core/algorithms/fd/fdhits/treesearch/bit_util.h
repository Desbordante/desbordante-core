#pragma once

#include <algorithm>

#include <boost/dynamic_bitset.hpp>

namespace algos::fd::fdhits::bit_util {
inline void SetTo(boost::dynamic_bitset<>& one, boost::dynamic_bitset<> const& other) {
    size_t original_size = one.size();
    one = other;
    if (one.size() < original_size) {
        one.resize(original_size, false);
    }
}

inline void SetToNot(boost::dynamic_bitset<>& one, boost::dynamic_bitset<> const& other) {
    size_t original_size = one.size();
    one = ~other;
    if (one.size() < original_size) {
        one.resize(original_size, false);
    }
}

inline void IntersectWithInverse(boost::dynamic_bitset<>& one,
                                 boost::dynamic_bitset<> const& other) {
    if (one.size() <= other.size()) {
        boost::dynamic_bitset<> tmp = other;
        tmp.resize(one.size());
        one -= tmp;
    } else {
        size_t original_size = one.size();
        one.resize(other.size());
        one -= other;
        one.resize(original_size, false);
    }
}

inline bool IsSubsetInverse(boost::dynamic_bitset<> const& one,
                            boost::dynamic_bitset<> const& other) {
    if (one.size() > other.size()) {
        if (one.find_next(other.size() - 1) != boost::dynamic_bitset<>::npos) {
            return false;
        }
    }
    boost::dynamic_bitset<> tmp = one;
    tmp.resize(other.size());
    return !tmp.intersects(other);
}

inline size_t GetIntersectionSize(boost::dynamic_bitset<> const& one,
                                  boost::dynamic_bitset<> const& other) {
    boost::dynamic_bitset<> tmp = one;
    tmp.resize(other.size());
    return (tmp & other).count();
}

}  // namespace algos::fd::fdhits::bit_util
