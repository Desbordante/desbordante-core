#include "sequence.h"
namespace algos::cmspade{
void Sequence::AddItemset(std::unique_ptr<Itemset> itemset){
    number_of_items_ += itemset->size();
    itemsets_.push_back(std::move(itemset));
}

void Sequence::AddItem(Item item){
    if(itemsets_.empty()){
        CreateNewItemset();
    }

    itemsets_.back()->AddItem(item);
    number_of_items_++;
}

std::unique_ptr<Itemset> Sequence::RemoveItemset(int itemset_index) {
    std::unique_ptr<Itemset> itemset = std::move(itemsets_[itemset_index]);
    itemsets_.erase(itemsets_.begin() + itemset_index);
    number_of_items_ -= itemset->size();
    return itemset;
}

Item Sequence::RemoveItem(int itemset_index, int item_index) {
    Item removed_item = itemsets_[itemset_index]->RemoveItem(item_index);
    number_of_items_--;
    return removed_item;
}

std::unique_ptr<Sequence> Sequence::CloneSequence() const{
    std::unique_ptr<Sequence> clone = std::make_unique<Sequence>(id_);
    for(const auto &itemset : itemsets_){
        clone->AddItemset(itemset->CloneItemset());
    }
    
    return clone;
}
} // namespace algos::cmspade