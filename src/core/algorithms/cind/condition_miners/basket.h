#pragma once

#include <list>
#include <unordered_map>

#include "item.h"

namespace algos::cind {
using ItemsInfo = std::unordered_map<Item, std::list<size_t>>;

struct Basket {
    bool is_included;
    ItemsInfo items{};

    bool IsContains(const Item& item) const {
        return items.contains(item);
    }
};
}  // namespace algos::cind
