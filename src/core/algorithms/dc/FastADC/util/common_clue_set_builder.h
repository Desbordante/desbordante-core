#pragma once

#include <bitset>
#include <unordered_map>
#include <vector>

namespace algos::fastadc {

/* Maximum supported number of bits in clue is 64 */
using Clue = std::bitset<64>;

struct ClueHash {
    std::size_t operator()(Clue const& clue) const noexcept {
        return clue.to_ullong();
    }
};

using ClueSet = std::unordered_map<Clue, int64_t, ClueHash>;

template <typename... Vectors>
ClueSet AccumulateClues(Vectors const&... vectors) {
    ClueSet clue_set;
    int64_t clue_zero_count = 0;

    auto insert_clues = [&](std::vector<Clue> const& clues) {
        for (auto const& clue : clues) {
            if (clue.none()) {
                ++clue_zero_count;
            } else {
                clue_set[clue]++;
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
