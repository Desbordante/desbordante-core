#pragma once

#include <algorithm>

#include "core/algorithms/md/hymd/lattice/md_node.h"
#include "core/algorithms/md/hymd/lattice/rhs.h"
#include "core/algorithms/md/hymd/md_lhs.h"

namespace algos::hymd::lattice {

struct MdLatticeNodeInfo {
    MdLhs lhs;
    MdNode* node;
};

}  // namespace algos::hymd::lattice
