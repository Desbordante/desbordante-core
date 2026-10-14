#pragma once

#include <cstddef>
#include <vector>

namespace pac::model {
using Tuple = std::vector<std::byte const*>;
using Tuples = std::vector<Tuple>;
}  // namespace pac::model
