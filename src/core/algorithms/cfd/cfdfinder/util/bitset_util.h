#pragma once

#include <cstddef>
#include <functional>

#include <boost/dynamic_bitset.hpp>

namespace algos::cfdfinder::util {
using BitSet = boost::dynamic_bitset<>;

void ForEachSetBit(BitSet const& bitset, std::function<void(std::size_t)>&& fun);
}  // namespace algos::cfdfinder::util
