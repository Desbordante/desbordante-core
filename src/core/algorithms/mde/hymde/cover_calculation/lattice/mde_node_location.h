#pragma once

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_node.h"

namespace algos::hymde::cover_calculation::lattice {
struct MdeNodeLocation {
    PathToNode lhs;
    MdeNode* node;
};
}  // namespace algos::hymde::cover_calculation::lattice
