#pragma once

#include <bitset>
#include <unordered_map>
#include <vector>

#include "../model/predicate.h"

namespace algos::fastadc {

/* Maximum supported number of bits in clue is kPredicateBits */
using Clue = std::bitset<kPredicateBits>;

struct ClueHash {
    std::size_t operator()(Clue const& clue) const noexcept {
        return clue.to_ullong();
    }
};

using ClueSet = std::unordered_map<Clue, int64_t, ClueHash>;

template <typename... Vectors>
ClueSet AccumulateClues(ClueSet& clue_set, Vectors const&... vectors) {
    clue_set.clear();
    int64_t clue_zero_count = 0;

    auto insert_clues = [&](std::vector<Clue> const& clues) {
        for (auto const& clue : clues) {
            if (clue.none()) {
                ++clue_zero_count;
            } else {
                auto [it, inserted] = clue_set.try_emplace(clue, 1);
                if (!inserted) it->second++;
            }
        }
    };

    (insert_clues(vectors), ...);

    if (clue_zero_count > 0) {
        clue_set[Clue(0)] = clue_zero_count;
    }

    return clue_set;
}
}  // namespace algos::fastadc
