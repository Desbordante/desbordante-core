#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::hymd {

enum class LevelDefinition : char {
    kCardinality = 0, /*define level as the set of mds with the same cardinality*/
    kLattice          /*define level as the whole lattice*/

};
}
