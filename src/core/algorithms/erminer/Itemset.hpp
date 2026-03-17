#pragma once
#include <vector>
#include <algorithm>

class Itemset {
public:
    std::vector<int> itemset;                    
    std::vector<int> transactionsIds;
    
    Itemset();
    Itemset(int item);
    Itemset(std::vector<int> items);
    
    int getAbsoluteSupport() const;
    
    std::vector<int> getItems() const;
    
    int get(int index) const;
    
    void setTIDs(const std::vector<int>& listTransactionIds);
    
    int size() const;
    
    std::vector<int> getTransactionsIds() const;
    
    Itemset cloneItemSetMinusAnItemset(const Itemset& itemsetToNotKeep) const;
    Itemset cloneItemSetMinusOneItem(int itemToRemove) const;
};