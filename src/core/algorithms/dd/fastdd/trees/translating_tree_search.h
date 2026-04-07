#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <vector>

#include "core/algorithms/dd/fastdd/trees/ntree_search.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"
#include "core/algorithms/dd/fastdd/util/bitset_translator.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class TranslatingTreeSearch {
private:
    NTreeSearch<Bitset> tree_;
    BitsetTranslator<Bitset> translator_;
    std::vector<Bitset> transformed_bitsets_;

public:
    TranslatingTreeSearch(std::vector<std::size_t> priorities, std::vector<Bitset> const& bitsets);

    void Insert(Bitset const& bitset) {
        tree_.Insert(translator_.Transform(bitset));
    }

    bool ContainsSubset(Bitset const& bitset) const {
        return tree_.ContainsSubset(translator_.Transform(bitset));
    }

    std::vector<Bitset> GetAndRemoveGeneralizations(Bitset const& bitset) {
        std::vector<Bitset> removed = tree_.GetAndRemoveGeneralizations(bitset);
        std::vector<Bitset> retransformed;
        retransformed.reserve(removed.size());
        BitsetTranslator<Bitset> const& translator = translator_;
        std::ranges::transform(
                removed, std::back_inserter(retransformed),
                [&](Bitset const& bitset) { return translator.Retransform(bitset); });
        return retransformed;
    }

    bool Compare(Bitset const& first_bitset, Bitset const& second_bitset) const {
        int diff = second_bitset.count() - first_bitset.count();
        return diff != 0
                       ? diff < 0
                       : translator_.Transform(second_bitset) < translator_.Transform(first_bitset);
    }

    void HandleInvalid(Bitset const& invalid_bitset);

    void ForEach(std::function<void(Bitset const&)> const& consumer) {
        tree_.ForEach([&consumer, this](Bitset const& bitset) {
            consumer(translator_.Retransform(bitset));
        });
    }
};

}  // namespace algos::dd
