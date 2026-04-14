#include "Itemset.hpp"

Itemset::Itemset(int item) {
    items.push_back(item);
}

void Itemset::addItem(int item) {
    items.push_back(item);
}

size_t Itemset::size() const {
    return items.size();
}

int Itemset::operator[](size_t i) const {
    return items[i];
}

const std::vector<int>& Itemset::getItems() const {
    return items;
}

Itemset Itemset::cloneItemSet() const {
    Itemset clone;
    for (int item : items) {
        clone.addItem(item);
    }
    return clone;
}

bool Itemset::contains(int item) const {
    return std::find(items.begin(), items.end(), item) != items.end();
}