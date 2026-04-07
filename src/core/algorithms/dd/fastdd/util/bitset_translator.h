#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "core/algorithms/dd/fastdd/util/bitset_concept.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class BitsetTranslator {
private:
    std::vector<std::size_t> indices_;

public:
    BitsetTranslator() = default;

    BitsetTranslator(std::vector<std::size_t> indices) : indices_(std::move(indices)) {}

    Bitset Transform(Bitset const& bitset) const {
        std::size_t const bitset_size = bitset.size();
        Bitset transformed_bitset(bitset_size);
        for (std::size_t i = 0; i != indices_.size(); ++i) {
            if (indices_[i] < bitset_size && bitset[indices_[i]]) {
                transformed_bitset.set(i);
            }
        }

        return transformed_bitset;
    }

    Bitset Retransform(Bitset const& bitset) const {
        Bitset retransformed_bitset(bitset.size());
        for (std::size_t index = bitset.find_first(); index != Bitset::npos;
             index = bitset.find_next(index)) {
            retransformed_bitset.set(indices_[index]);
        }

        return retransformed_bitset;
    }
};

}  // namespace algos::dd
