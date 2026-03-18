#pragma once
#include "types.h"
#include "item.h"
#include "itemset.h"
namespace algos::cmspade{
class Sequence{
private:
    SequenceId id_;
    std::vector<std::unique_ptr<Itemset>> itemsets_;
    int number_of_items_ = 0;
public:
    Sequence(SequenceId id) : id_(id){}

    Sequence(Sequence&& other) = default;
    Sequence& operator=(Sequence&& other) = default;
    
    Sequence(const Sequence&) = delete;
    Sequence& operator=(const Sequence&) = delete;

    void AddItemset(std::unique_ptr<Itemset> itemset);

    void AddItem(Item item);
    
    std::unique_ptr<Itemset> RemoveItemset(int itemset_index);

    Item RemoveItem(int itemset_index, int item_index);

    void CreateNewItemset() { itemsets_.emplace_back(std::make_unique<Itemset>()); }

    int GetId() const { return id_; }
    void SetId(int id){ id_ = id; }

    const std::vector<std::unique_ptr<Itemset>>& GetItemsets() const { return itemsets_; }

    size_t size() const { return itemsets_.size(); }
    int length() const { return number_of_items_; }

    std::unique_ptr<Sequence> CloneSequence() const;
};
} // namespace algos::cmspade