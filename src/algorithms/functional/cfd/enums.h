#pragma once

#include <enum.h>

namespace algos::cfd {

// Defines what kind of lattice traversal will be used in pattern mining part of the algorithm.
BETTER_ENUM(Substrategy, char,
            dfs = 0,  // dfs lattice traversal
            bfs       // bfs lattice traversal
);
}  // namespace algos::cfd
