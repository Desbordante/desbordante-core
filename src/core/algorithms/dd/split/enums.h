#pragma once

#include <enum.h>

namespace algos::dd {

// Defines what strategy of reducing dependencies will be used in the algorithm
BETTER_ENUM(Reduce, char,
            Negative = 0,  // negative pruning reduce
            Hybrid,        // hybrid pruning reduce
            IEHybrid       // instance exclusion reduce (currently, the fastest)
);
}  // namespace algos::dd
