#include "core/algorithms/cfd/cfdfinder/util/lhs_utils.h"

#include <cstddef>
#include <list>

#include "core/util/bitset_utils.h"

namespace algos::cfdfinder::utils {

std::list<BitSet> GenerateLhsSubsets(BitSet const& lhs) {
    std::list<BitSet> subsets;

    util::ForEachIndex(lhs, [&](size_t attr) {
        auto subset = lhs;
        subset.flip(attr);

        if (subset.any()) {
            subsets.push_back(std::move(subset));
        }
    });

    return subsets;
}

std::list<BitSet> GenerateLhsSupersets(BitSet const& lhs) {
    std::list<BitSet> supersets;
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (!lhs.test(i)) {
            BitSet superset(lhs);
            superset.set(i);
            supersets.push_back(std::move(superset));
        }
    }
    return supersets;
}
}  // namespace algos::cfdfinder::utils
