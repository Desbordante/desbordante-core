#pragma once

#include <bitset>
#include <concepts>
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

template <typename T>
concept HasNone = requires(T t) {
    t.none();
};

template <typename ClueT>
[[nodiscard]] inline bool IsZeroClue(ClueT const& clue) {
    if constexpr (HasNone<ClueT>) {
        return clue.none();
    } else {
        return clue == 0;
    }
}

template <typename ClueT>
struct ClueHashT {
    std::size_t operator()(ClueT const& clue) const noexcept {
        return std::hash<ClueT>{}(clue);
    }
};

template <typename ClueT>
#if UNORDERED_FLAT_MAP_AVAILABLE
using ClueSetT = boost::unordered::unordered_flat_map<ClueT, int64_t, ClueHashT<ClueT>>;
#else
using ClueSetT = std::unordered_map<ClueT, int64_t, ClueHashT<ClueT>>;
#endif

/* Maximum supported number of bits in clue is kMaxPredicateBits */
using Clue = model::Bitset<kMaxPredicateBits>;
using ClueSet = ClueSetT<Clue>;

template <typename ClueT, typename... Vectors>
ClueSetT<ClueT> AccumulateClues(ClueSetT<ClueT>& clue_set, Vectors const&... vectors) {
    clue_set.clear();
    int64_t clue_zero_count = 0;

    auto insert_clues = [&](std::vector<ClueT> const& clues) {
        for (auto const& clue : clues) {
            if (IsZeroClue(clue)) {
                ++clue_zero_count;
            } else {
                auto [it, inserted] = clue_set.emplace(clue, 1);
                if (!inserted) {
                    it->second++;
                }
            }
        }
    };

    (insert_clues(vectors), ...);

    if (clue_zero_count > 0) {
        clue_set[ClueT(0)] = clue_zero_count;
    }

    return clue_set;
}

}  // namespace algos::fastadc
