#include "lhs_utils.h"

namespace algos::cfdfinder::util {

std::list<BitSet> GenerateLhsSubsets(BitSet const& lhs) {
    std::list<BitSet> subsets;

    for (size_t i = lhs.find_first(); i != BitSet::npos; i = lhs.find_next(i)) {
        auto subset = lhs;
        subset.flip(i);

        if (subset.any()) {
            subsets.push_back(std::move(subset));
        }
    }

    return subsets;
}

void AddLhsSubsets(Candidate const& candidate, std::set<Candidate>& level) {
    for (auto&& subset : GenerateLhsSubsets(candidate.lhs_)) {
        level.emplace(std::move(subset), candidate.rhs_);
    }
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
}  // namespace algos::cfdfinder::util