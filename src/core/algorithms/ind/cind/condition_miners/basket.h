#pragma once

#include <unordered_set>

#include "itemset.h"

namespace algos::cind {
struct Basket {
    bool is_included;
    std::unordered_set<Item> items{};
};
}  // namespace algos::cind
