#pragma once

#include <easylogging++.h>

#include "cross_clue_set_builder.h"
#include "single_clue_set_builder.h"

namespace algos::fastadc {

inline ClueSet BuildClueSet(std::vector<PliShard> const& pliShards, PredicatePacks const& packs) {
    ClueSet clue_set;
    size_t task_count = (pliShards.size() * (pliShards.size() + 1)) / 2;
    LOG(DEBUG) << "  [CLUE] task count: " << task_count;

    for (size_t i = 0; i < pliShards.size(); i++) {
        for (size_t j = i; j < pliShards.size(); j++) {
            ClueSet partial_clue_set;

            if (i == j) {
                partial_clue_set = SingleClueSetBuilder{pliShards[i]}.BuildClueSet(packs);
            } else {
                partial_clue_set =
                        CrossClueSetBuilder{pliShards[i], pliShards[j]}.BuildClueSet(packs);
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
