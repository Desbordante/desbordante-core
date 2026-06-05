#pragma once

#include "core/algorithms/dc/FastADC/model/predicate.h"

namespace algos::fastadc {

struct Evidence {
    int64_t count;
    PredicateBitset evidence;

    Evidence(PredicateBitset const& satisfied, int64_t count,
             PredicateBitset const& cardinality_mask,
             std::vector<PredicateBitset> const& correction_map)
        : count(count) {
        evidence = cardinality_mask;

        for (size_t pos = 0; pos < satisfied.size(); ++pos) {
            if (satisfied.test(pos)) {
                evidence ^= correction_map[pos];
            }
        }
    }

    Evidence(PredicateBitset bit_set, int64_t count) : count(count), evidence(bit_set) {}
};

}  // namespace algos::fastadc
