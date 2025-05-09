#pragma once

#include <cstddef>        // for size_t
#include <unordered_set>  // for unordered_set

namespace config {
template <typename T>
class CommonOption;
}

namespace config {
extern CommonOption<std::unordered_set<size_t>> const kDeleteStatementsOpt;
}  // namespace config
