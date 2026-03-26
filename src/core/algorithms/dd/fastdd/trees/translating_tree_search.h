#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/trees/ntree_search.h"
#include "core/algorithms/dd/fastdd/util/bitset_translator.h"

namespace algos::dd {

class TranslatingTreeSearch {
private:
    NTreeSearch tree_;
    BitsetTranslator translator_;
    std::vector<boost::dynamic_bitset<>> transformed_bitsets_;

public:
    TranslatingTreeSearch(std::vector<std::size_t> priorities,
                          std::vector<boost::dynamic_bitset<>> const& bitsets);

    void Insert(boost::dynamic_bitset<> const& bitset) {
        tree_.Insert(translator_.Transform(bitset));
    }

    bool ContainsSubset(boost::dynamic_bitset<> const& bitset) const {
        return tree_.ContainsSubset(translator_.Transform(bitset));
    }

    std::vector<boost::dynamic_bitset<>> GetAndRemoveGeneralizations(
            boost::dynamic_bitset<> const& bitset) {
        std::vector<boost::dynamic_bitset<>> removed = tree_.GetAndRemoveGeneralizations(bitset);
        std::vector<boost::dynamic_bitset<>> retransformed;
        retransformed.reserve(removed.size());
        BitsetTranslator const& translator = translator_;
        std::ranges::transform(removed, std::back_inserter(retransformed),
                               [&translator](boost::dynamic_bitset<> const& bitset) {
                                   return translator.Retransform(bitset);
                               });
        return retransformed;
    }

    bool Compare(boost::dynamic_bitset<> const& first_bitset,
                 boost::dynamic_bitset<> const& second_bitset) const {
        int diff = second_bitset.count() - first_bitset.count();
        return diff != 0
                       ? diff < 0
                       : translator_.Transform(second_bitset) < translator_.Transform(first_bitset);
    }

    void HandleInvalid(boost::dynamic_bitset<> const& invalid_bitset);

    void ForEach(std::function<void(boost::dynamic_bitset<> const&)> const& consumer) {
        tree_.ForEach([&consumer, this](boost::dynamic_bitset<> const& bitset) {
            consumer(translator_.Retransform(bitset));
        });
    }
};

}  // namespace algos::dd
