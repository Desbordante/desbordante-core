#include "Itemset.hpp"
#include <algorithm>

Itemset::Itemset() = default;

Itemset::Itemset(int item) {
    itemset.push_back(item);
}

Itemset::Itemset(std::vector<int> items) : itemset(std::move(items)) {
    std::sort(itemset.begin(), itemset.end());
}

int Itemset::getAbsoluteSupport() const {
    return static_cast<int>(transactionsIds.size());
}

std::vector<int> Itemset::getItems() const {
    return itemset;
}

int Itemset::get(int index) const {
    return itemset[index];
}

void Itemset::setTIDs(const std::vector<int>& listTransactionIds) {
    transactionsIds = listTransactionIds;
    std::sort(transactionsIds.begin(), transactionsIds.end());
    transactionsIds.erase(std::unique(transactionsIds.begin(), transactionsIds.end()), transactionsIds.end());
}

int Itemset::size() const {
    return static_cast<int>(itemset.size());
}

std::vector<int> Itemset::getTransactionsIds() const {
    return transactionsIds;
}

Itemset Itemset::cloneItemSetMinusAnItemset(const Itemset& itemsetToNotKeep) const {
    std::vector<int> newItemset;
    
    for (int item : itemset) {
        bool found = false;
        for (int excludeItem : itemsetToNotKeep.itemset) {
            if (item == excludeItem) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            newItemset.push_back(item);
        }
    }
    
    return Itemset(newItemset);
}

Itemset Itemset::cloneItemSetMinusOneItem(int itemToRemove) const {
    std::vector<int> newItemset;
    
    for (int item : itemset) {
        if (item != itemToRemove) {
            newItemset.push_back(item);
        }
    }
    
    return Itemset(newItemset);
}