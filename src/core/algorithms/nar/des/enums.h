#pragma once

#include "core/util/better_enum_with_visibility.h"

namespace algos::des {
BETTER_ENUM(DifferentialStrategy, char, rand1Bin = 0, rand1Exp, randToBest1Exp, best2Exp, rand2Exp,
            best1Bin, best1Exp, randToBest1Bin, best2Bin,
            rand2Bin);  // TODO: add descriptions of each strategy
}  // namespace algos::des
