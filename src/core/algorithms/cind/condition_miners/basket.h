#pragma once

#include <unordered_map>
#include <vector>

#include "item.h"

namespace algos::cind {
using ItemsInfo = std::unordered_map<Item, std::vector<size_t>>;

struct Basket {
    bool is_included{false};
    ItemsInfo items{};

    bool IsContains(Item const& item) const {
        return items.contains(item);
    }
};
}  // namespace algos::cind
