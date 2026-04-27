#pragma once
#include <vector>
#include <algorithm>

class Itemset {
private:
    std::vector<int> items;

public:
    Itemset() = default;
    explicit Itemset(int item);
    void addItem(int item);
    size_t size() const;
    int operator[](size_t i) const;
    const std::vector<int>& getItems() const;
    Itemset cloneItemSet() const;
    bool contains(int item) const;
};