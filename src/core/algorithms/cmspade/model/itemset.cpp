#include "itemset.h"

namespace algos::cmspade{
void Itemset::AddItem(Item item){ items_.emplace_back(std::move(item)); }

Item Itemset::RemoveItem(int index){
    Item item = std::move(items_[index]);
    items_.erase(items_.begin() + index);
    return item;
}

std::unique_ptr<Itemset> Itemset::CloneItemset() const {
    std::unique_ptr<Itemset> clone = std::make_unique<Itemset>();
    clone->items_ = items_;
    return clone;
}
} // namespace algos::cmspade