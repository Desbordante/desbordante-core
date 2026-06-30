#pragma once

#include <functional>

#include <boost/dynamic_bitset.hpp>

#include "core/model/index.h"

namespace algos::fd {
using BitsetAndIndexResultReporter = std::function<void(boost::dynamic_bitset<>, model::Index)>;
}  // namespace algos::fd
