#pragma once
#include <memory>
#include <utility>
#include <vector>

#include "item.h"
#include "types.h"

namespace algos::cmspade {
class Itemset {
private:
    std::vector<Item> items_;

public:
    Itemset() = default;
    ~Itemset() = default;

    Itemset(Itemset const&) = default;
    Itemset& operator=(Itemset const&) = default;

    Itemset(Itemset&&) = default;
    Itemset& operator=(Itemset&&) = default;

    void AddItem(Item item);
    Item RemoveItem(int index);

    std::vector<Item> const& GetItems() const {
        return items_;
    }

    Item GetItem(int index) const {
        return items_[index];
    }

    std::size_t Size() const {
        return items_.size();
    }

    bool Empty() const {
        return items_.empty();
    }

    std::unique_ptr<Itemset> CloneItemset() const;
};
}  // namespace algos::cmspade