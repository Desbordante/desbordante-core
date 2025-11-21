#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::cfd {

enum class Substrategy : char {
    dfs = 0,  // dfs lattice traversal
    bfs       // bfs lattice traversal
};
}  // namespace algos::cfd
