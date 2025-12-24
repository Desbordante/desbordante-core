#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::des {
enum class DifferentialStrategy : char {
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
