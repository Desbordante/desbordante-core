#pragma once

#include <algorithm>

#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/md_lhs.h"

namespace algos::hymd::lattice {

struct MdLatticeNodeInfo {
    MdLhs lhs;
    Rhs* rhs;

    void ZeroRhs() {
        rhs->ZeroRhs();
    }
};

}  // namespace algos::hymd::lattice
