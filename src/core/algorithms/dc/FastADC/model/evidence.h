#include "dc/FastADC/util/common_clue_set_builder.h"
#include "predicate.h"

namespace algos::fastadc {

struct Evidence {
    int64_t count;
    PredicateBitset evidence;

    Evidence(Clue satisfied, int64_t count, PredicateBitset const& cardinalityMask,
             std::vector<PredicateBitset> const& correctionMap)
        : count(count) {
        evidence = cardinalityMask;

        Clue tmp = satisfied;
        size_t pos = 0;
        while (tmp.any()) {
            if (tmp.test(0)) {
                evidence ^= correctionMap[pos];
            }
            tmp >>= 1;
            pos++;
        }
    }

    Evidence(Clue bitSet, int64_t count) : count(count), evidence(bitSet) {}
};

}  // namespace algos::fastadc
