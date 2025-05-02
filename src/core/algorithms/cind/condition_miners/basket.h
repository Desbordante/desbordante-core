#pragma once

#include <unordered_map>

#include "item.h"

namespace algos::cind {
struct Basket {
    bool is_included;
    std::unordered_map<Item, std::vector<size_t>> items{};

    bool IsContains(const Item& item) const {
        return items.contains(item);
    }
};
}  // namespace algos::cind
