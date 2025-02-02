#pragma once

#include <unordered_set>

#include "itemset.h"

namespace algos::cind {
struct Basket {
    bool is_included;
    std::unordered_set<Item> items{};

    bool IsContains(const Itemset& itemset) const {
        for (size_t index = 0; index < itemset.GetSize(); ++index) {
            if (const auto &item = itemset.GetItem(index); !items.contains(item)) {
                return false;
            }
        }
        return true;
    }
};
}  // namespace algos::cind
