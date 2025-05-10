#pragma once

#include <boost/dynamic_bitset.hpp>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lattice.h"

namespace algos::hymde::cover_calculation {
struct ValidationSelection {
    lattice::MdeLattice::ValidationUpdater* updater;
    boost::dynamic_bitset<> rhs_indices_to_validate;
};
}  // namespace algos::hymde::cover_calculation
