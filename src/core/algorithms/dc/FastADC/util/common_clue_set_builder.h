#pragma once

#include <bitset>
#include <functional>
#include <unordered_map>
#include <vector>

#include <boost/version.hpp>

#define UNORDERED_FLAT_MAP_AVAILABLE (BOOST_VERSION >= 108100)

#if UNORDERED_FLAT_MAP_AVAILABLE
#include <boost/unordered/unordered_flat_map.hpp>
#endif

#include "core/algorithms/dc/FastADC/model/predicate.h"
#include "core/model/types/bitset.h"

namespace algos::fastadc {

/* Maximum supported number of bits in clue is kPredicateBits */
using Clue = model::Bitset<kPredicateBits>;

struct ClueHash {
    std::size_t operator()(Clue const& clue) const noexcept {
        return std::hash<Clue>{}(clue);
    }
};

#if UNORDERED_FLAT_MAP_AVAILABLE
using ClueSet = boost::unordered::unordered_flat_map<Clue, int64_t, ClueHash>;
#else
using ClueSet = std::unordered_map<Clue, int64_t, ClueHash>;
#endif

template <typename... Vectors>
ClueSet AccumulateClues(ClueSet& clue_set, Vectors const&... vectors) {
    clue_set.clear();
    int64_t clue_zero_count = 0;

    auto insert_clues = [&](std::vector<Clue> const& clues) {
        for (auto const& clue : clues) {
            if (clue.none()) {
                ++clue_zero_count;
            } else {
                auto [it, inserted] = clue_set.emplace(clue, 1);
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
