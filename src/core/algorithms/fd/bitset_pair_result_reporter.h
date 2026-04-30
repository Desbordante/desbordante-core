#pragma once

#include <functional>

#include <boost/dynamic_bitset.hpp>

namespace algos::fd {
using BitsetPairResultReporter =
        std::function<void(boost::dynamic_bitset<>, boost::dynamic_bitset<>)>;
}  // namespace algos::fd
