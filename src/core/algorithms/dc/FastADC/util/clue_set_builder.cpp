#include "dc/FastADC/util/clue_set_builder.h"

#include "dc/FastADC/util/cross_clue_set_builder.h"
#include "dc/FastADC/util/single_clue_set_builder.h"

namespace algos::fastadc {

ClueSet BuildClueSet(std::vector<PliShard> const& pliShards, PredicatePacks const& packs) {
    ClueSet clue_set;
    ClueSet partial_clue_set;
    size_t task_count = (pliShards.size() * (pliShards.size() + 1)) / 2;
    LOG(DEBUG) << "  [CLUE] task count: " << task_count;

    // Range of all pliShards is equal, so it's safe to pass a pre-allocated vector of
    // pliShards[0]'s range
    size_t range = pliShards[0].Range();
    size_t evidence_count = range * range;

    clue_set.reserve(range * 2);
    partial_clue_set.reserve(range * 2);

    std::vector<Clue> forward_clues(evidence_count, 0);
    std::vector<Clue> reverse_clues(evidence_count, 0);

    for (size_t i = 0; i < pliShards.size(); i++) {
        for (size_t j = i; j < pliShards.size(); j++) {
            if (i == j) {
                SingleClueSetBuilder{pliShards[i]}.BuildClueSet(packs, forward_clues,
                                                                partial_clue_set);
            } else {
                CrossClueSetBuilder{pliShards[i], pliShards[j]}.BuildClueSet(
                        packs, forward_clues, reverse_clues, partial_clue_set);
            }

            for (auto const& [clue, count] : partial_clue_set) {
                auto [it, inserted] = clue_set.try_emplace(clue, count);
                if (!inserted) {
                    it->second += count;
                }
            }
        }
    }

    return clue_set;
}

}  // namespace algos::fastadc
