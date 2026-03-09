#pragma once

#include "core/algorithms/dc/FastADC/model/predicate.h"

namespace algos::fastadc {

struct Evidence {
    int64_t count;
    PredicateBitset evidence;

    Evidence(PredicateBitset const& satisfied, int64_t count, PredicateBitset const& cardinalityMask,
             std::vector<PredicateBitset> const& correctionMap)
        : count(count) {
        evidence = cardinalityMask;

        for (size_t pos = 0; pos < satisfied.size(); ++pos) {
            if (satisfied.test(pos)) {
                evidence ^= correctionMap[pos];
            }
        }
    }

    Evidence(PredicateBitset bitSet, int64_t count) : count(count), evidence(bitSet) {}
};

}  // namespace algos::fastadc
