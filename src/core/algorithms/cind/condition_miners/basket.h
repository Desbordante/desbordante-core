#pragma once

#include <unordered_set>

#include "item.h"

namespace algos::cind {
struct Basket {
    bool is_included;
    std::unordered_set<Item> items{};

    bool IsContains(const Item& item) const {
        return items.contains(item);
    }
};
}  // namespace algos::cind
