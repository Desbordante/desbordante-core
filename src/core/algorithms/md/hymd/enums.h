#pragma once

#include "core/util/export.h"

namespace algos::hymd {

enum class DESBORDANTE_EXPORT LevelDefinition : char {
    kCardinality = 0, /*define level as the set of mds with the same cardinality*/
    kLattice          /*define level as the whole lattice*/

};
}
