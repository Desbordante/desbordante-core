#include "core/algorithms/dd/fastdd/trees/translating_tree_search.h"

#include <algorithm>
#include <numeric>
#include <utility>

namespace algos::dd {

TranslatingTreeSearch::TranslatingTreeSearch(std::vector<std::size_t> priorities,
                                             std::vector<boost::dynamic_bitset<>> const& bitsets)
    : tree_(bitsets[0].size()) {
    std::vector<std::size_t> indices(priorities.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::ranges::sort(indices, [&priorities](std::size_t i, std::size_t j) {
        return priorities[i] > priorities[j];
    });

    translator_ = BitsetTranslator(std::move(indices));
    transformed_bitsets_ = std::vector<boost::dynamic_bitset<>>(bitsets.size());
    BitsetTranslator const& translator = translator_;
    std::ranges::transform(bitsets, transformed_bitsets_.begin(),
                           [&translator](boost::dynamic_bitset<> const& bitset) {
                               return translator.Transform(bitset);
                           });
}

void TranslatingTreeSearch::HandleInvalid(boost::dynamic_bitset<> const& invalid_bitset) {
    boost::dynamic_bitset<> transformed_invalid_bitset = translator_.Transform(invalid_bitset);
    std::vector<util::DynamicBitset<>> static_removed =
            tree_.GetAndRemoveGeneralizations(transformed_invalid_bitset);
    std::vector<boost::dynamic_bitset<>> removed;
    removed.reserve(static_removed.size());
    std::ranges::transform(
            static_removed, std::back_inserter(removed),
            [&](util::DynamicBitset<> const& bitset) { return bitset.to_boost_dynamic_bitset(); });

    for (std::size_t i = 0; i != removed.size(); ++i) {
        for (std::size_t j = 0; j != transformed_bitsets_.size(); ++j) {
            boost::dynamic_bitset<> cur_bitset = removed[i];
            cur_bitset &= transformed_bitsets_[j];

            if (cur_bitset.none()) {
                boost::dynamic_bitset<> valid_bitset = transformed_bitsets_[j];
                valid_bitset &= (~transformed_invalid_bitset);
                for (std::size_t index = valid_bitset.find_first();
                     index != boost::dynamic_bitset<>::npos;
                     index = valid_bitset.find_next(index)) {
                    boost::dynamic_bitset<> bitset_to_add = removed[i];
                    bitset_to_add.set(index);
                    if (!tree_.ContainsSubset(bitset_to_add)) {
                        tree_.Insert(bitset_to_add);
                    }
                }
            }
        }
    }
}

}  // namespace algos::dd
