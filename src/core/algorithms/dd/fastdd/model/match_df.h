#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "core/algorithms/dd/fastdd/util/bitset_concept.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class MatchDF {
private:
    Bitset bitset_;

public:
    MatchDF(std::size_t clue, std::size_t const bitset_size,
            std::vector<std::vector<Bitset>> const& count_to_predicates,
            std::vector<std::size_t> const& bases)
        : bitset_(bitset_size) {
        for (std::size_t i = count_to_predicates.size(); i != 0; --i) {
            std::size_t const offset = clue / bases[i - 1];
            clue %= bases[i - 1];
            bitset_ |= count_to_predicates[i - 1][offset];
        }
    }

    explicit MatchDF(Bitset bitset) : bitset_(std::move(bitset)) {}

    Bitset const& GetBitset() const noexcept {
        return bitset_;
    }
};

}  // namespace algos::dd
