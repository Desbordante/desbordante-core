#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::dd {

// Defines what strategy of reducing dependencies will be used in the algorithm
enum class Reduce : char {
    kNegative = 0,  // negative pruning reduce
    kHybrid,        // hybrid pruning reduce
    kIeHybrid       // instance exclusion reduce (currently, the fastest)

};
}  // namespace algos::dd
