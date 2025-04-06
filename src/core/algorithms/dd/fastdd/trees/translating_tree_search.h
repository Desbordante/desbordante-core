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

    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = boost::dynamic_bitset<>;
        using pointer = value_type const*;
        using reference = value_type const&;

        Iterator(NTreeSearch::Iterator it, BitsetTranslator const& translator)
            : it_(it), translator_(translator) {}

        reference operator*() const {
            cur_bitset_ = translator_.Retransform(*it_);
            return cur_bitset_;
        }

        Iterator& operator++() {
            ++it_;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);

            return tmp;
        }

        friend bool operator==(Iterator const& a, Iterator const& b) {
            return a.it_ == b.it_;
        }

        friend bool operator!=(Iterator const& a, Iterator const& b) {
            return !(a == b);
        }

    private:
        NTreeSearch::Iterator it_;
        BitsetTranslator const& translator_;
        boost::dynamic_bitset<> mutable cur_bitset_;  // Looks weird. Is there a better way?
    };

    Iterator begin() {
        return Iterator{tree_.begin(), translator_};
    }

    Iterator end() {
        return Iterator{tree_.end(), translator_};
    }
};

}  // namespace algos::dd
