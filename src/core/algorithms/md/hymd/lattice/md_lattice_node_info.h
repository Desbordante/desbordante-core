#pragma once

#include <algorithm>

#include "algorithms/md/hymd/lattice/md_node.h"
#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/md_lhs.h"

namespace algos::hymd::lattice {

struct MdLatticeNodeInfo {
    MdLhs lhs;
    MdNode* node;

    void ZeroRhs() {
        node->rhs.ZeroRhs();
    }
};

}  // namespace algos::hymd::lattice
