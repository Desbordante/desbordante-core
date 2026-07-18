#pragma once

#include <magic_enum/magic_enum.hpp>

#include "core/util/export.h"

namespace algos::cfd {

enum class DESBORDANTE_EXPORT Substrategy : char {
    kDfs = 0,  // dfs lattice traversal
    kBfs       // bfs lattice traversal
};
}  // namespace algos::cfd
