#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::cfd {

enum class Substrategy : char {
    kDfs = 0,  // dfs lattice traversal
    kBfs       // bfs lattice traversal
};
}  // namespace algos::cfd
