#pragma once

#include <enum.h>

namespace algos::hymd {

BETTER_ENUM(LevelDefinition, char,
            cardinality = 0, /*define level as the set of mds with the same cardinality*/
            lattice          /*define level as the whole lattice*/
)

}
