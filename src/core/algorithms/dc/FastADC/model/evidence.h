#include "core/algorithms/dc/FastADC/model/predicate.h"
#include "core/algorithms/dc/FastADC/util/common_clue_set_builder.h"

namespace algos::fastadc {

struct Evidence {
    int64_t count;
    PredicateBitset evidence;

    Evidence(Clue satisfied, int64_t count, PredicateBitset const& cardinalityMask,
             std::vector<PredicateBitset> const& correctionMap)
        : count(count) {
        evidence = cardinalityMask;

        Clue tmp = satisfied;
        for (size_t pos = 0; tmp.any(); ++pos) {
            if (tmp.test(0)) {
                evidence ^= correctionMap[pos];
            }
            tmp >>= 1;
        }
    }

    Evidence(Clue bitSet, int64_t count) : count(count), evidence(bitSet) {}
};

}  // namespace algos::fastadc
