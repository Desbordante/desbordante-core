#pragma once

#include "core/util/export.h"

namespace algos::des {
enum class DESBORDANTE_EXPORT DifferentialStrategy : char {
    kRand1Bin = 0,
    kRand1Exp,
    kRandToBest1Exp,
    kBest2Exp,
    kRand2Exp,
    kBest1Bin,
    kBest1Exp,
    kRandToBest1Bin,
    kBest2Bin,
    kRand2Bin
};  // TODO: add descriptions of each strategy
}  // namespace algos::des
