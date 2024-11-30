#pragma once

#include <unordered_set>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "model/index.h"

namespace algos::hymd::lattice {

struct ValidationInfo {
    MdLattice::MdVerificationMessenger* messenger;
    boost::dynamic_bitset<> rhs_indices_to_validate;
};

}  // namespace algos::hymd::lattice
