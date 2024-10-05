#pragma once

#include <bitset>
#include <unordered_map>
#include <vector>

namespace algos::fastadc {

/* Maximum supported number of bits in clue is 64 */
using Clue = std::bitset<64>;
using ClueSet = std::unordered_map<Clue, int64_t>;

template <typename... Vectors>
inline ClueSet AccumulateClues(Vectors const&... vectors) {
    ClueSet clue_set;
    auto insert_clues = [&clue_set](std::vector<Clue> const& clues) {
        for (auto const& clue : clues) {
            clue_set[clue]++;
        }
    };
    (insert_clues(vectors), ...);
    return clue_set;
}
}  // namespace algos::fastadc
