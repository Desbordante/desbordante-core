#pragma once
#include <vector>
#include <memory>
#include <utility>

#include "item.h"
#include "types.h"
namespace algos::cmspade{
class Itemset{
private:
    std::vector<Item> items_;

public:
    Itemset() = default;
    ~Itemset() = default;

    Itemset(const Itemset&) = default;
    Itemset& operator=(const Itemset&) = default;
    
    Itemset(Itemset&&) = default;
    Itemset& operator=(Itemset&&) = default;

    void AddItem(Item item);
    Item RemoveItem(int index);

    const std::vector<Item> &GetItems() const { return items_; }
    Item GetItem(int index) const { return items_[index]; }

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    
    std::unique_ptr<Itemset> CloneItemset() const;

};
} // namespace algos::cmspade