#pragma once
#include "Itemset.hpp"
#include <vector>
#include <string>
#include <sstream>  
#include <iostream>

class Prefix {
private:
    std::vector<Itemset> itemsets;
public:
    Prefix() = default;

    void addItemset(const Itemset& itemset) {
        itemsets.push_back(itemset);
    }

    Prefix cloneSequence() const {
        Prefix clone;
        for (const auto& itemset : itemsets) {
            clone.addItemset(itemset.cloneItemSet());
        }
        return clone;
    }

    void print() const {
        std::cout << toString();
    }

    std::string toString() const {
        std::ostringstream r;
    r << "<";
    for (size_t i = 0; i < itemsets.size(); ++i) {
        if (i > 0) r << " ";
        r << "{";
        const auto& items = itemsets[i].getItems();
        for (size_t j = 0; j < items.size(); ++j) {
            if (j > 0) r << " ";
            r << items[j];
        }
        r << "}";
    }
    r << ">";
    return r.str();
    }

    std::vector<Itemset>& getItemsets()
    {
        return itemsets;
    }
    const std::vector<Itemset>& getItemsets() const
    {
        return itemsets;
    }

    const Itemset& get(size_t index) const {
        return itemsets[index];
    }

    int getIthItem(int i) const {
        for (size_t j = 0; j < itemsets.size(); ++j) {
            if (i < static_cast<int>(itemsets[j].size())) {
                return itemsets[j][i];
            }
            i -= static_cast<int>(itemsets[j].size());
        }
        return -1;
    }

    size_t size() const {
        return itemsets.size();
    }

    int getItemOccurencesTotalCount() const {
        int count = 0;
        for (const auto& itemset : itemsets) {
            count += static_cast<int>(itemset.size());
        }
        return count;
    }

    bool containsItem(int item) const {
        for (const auto& itemset : itemsets) {
            if (itemset.contains(item)) {
                return true;
            }
        }
        return false;
    }
};