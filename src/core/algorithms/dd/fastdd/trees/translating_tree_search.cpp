#include "core/algorithms/dd/fastdd/trees/translating_tree_search.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/util/static_bitset.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
TranslatingTreeSearch<Bitset>::TranslatingTreeSearch(std::vector<std::size_t> priorities,
                                                     std::vector<Bitset> const& bitsets)
    : tree_(bitsets[0].size()) {
    std::vector<std::size_t> indices(priorities.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::ranges::sort(indices, [&priorities](std::size_t i, std::size_t j) {
        return priorities[i] > priorities[j];
    });

    translator_ = BitsetTranslator<Bitset>(std::move(indices));
    transformed_bitsets_.reserve(bitsets.size());
    BitsetTranslator<Bitset> const& translator = translator_;
    std::ranges::transform(
            bitsets, std::back_inserter(transformed_bitsets_),
            [&translator](Bitset const& bitset) { return translator.Transform(bitset); });
}

template <BoostDynamicBitsetCompatible Bitset>
inline void TranslatingTreeSearch<Bitset>::HandleInvalid(Bitset const& invalid_bitset) {
    Bitset transformed_invalid_bitset = translator_.Transform(invalid_bitset);
    std::vector<Bitset> removed = tree_.GetAndRemoveGeneralizations(transformed_invalid_bitset);

    for (std::size_t i = 0; i != removed.size(); ++i) {
        for (std::size_t j = 0; j != transformed_bitsets_.size(); ++j) {
            Bitset cur_bitset = removed[i];
            cur_bitset &= transformed_bitsets_[j];

            if (cur_bitset.none()) {
                Bitset valid_bitset = transformed_bitsets_[j];
                valid_bitset -= transformed_invalid_bitset;
                for (std::size_t index = valid_bitset.find_first(); index != Bitset::npos;
                     index = valid_bitset.find_next(index)) {
                    Bitset bitset_to_add = removed[i];
                    bitset_to_add.set(index);
                    if (!tree_.ContainsSubset(bitset_to_add)) {
                        tree_.Insert(bitset_to_add);
                    }
                }
            }
        }
    }
}

// Explicit instantiation is used instead of implementing the methods in header file for
// optimization reasons. Including large number of header files somehow interferes with the
// compilation process, resulting in degraded performance.
template class TranslatingTreeSearch<StaticBitset<32>>;
template class TranslatingTreeSearch<StaticBitset<64>>;
template class TranslatingTreeSearch<StaticBitset<128>>;
template class TranslatingTreeSearch<boost::dynamic_bitset<>>;

}  // namespace algos::dd
