#include <vector>
#include <easylogging++.h>

#include "dc/common_clue_set_builder.h"

namespace model {

struct Evidence {
    int64_t count;
    Clue clue;
    PredicateBitset evidence;

    Evidence(Clue satisfied, int64_t count, PredicateBitset const& cardinalityMask,
             std::vector<Clue> const& correctionMap)
        : count(count), clue(satisfied) {
        evidence = cardinalityMask;

        Clue tmp = clue;
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

}  // namespace model
