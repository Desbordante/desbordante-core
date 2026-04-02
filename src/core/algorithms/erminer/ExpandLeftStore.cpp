#include "ExpandLeftStore.hpp"
#include <functional>
#include <algorithm>

void ExpandLeftStore::registerRule(const LeftRule& leftRule, 
    const std::vector<int>& itemsetJ, 
    const std::vector<int>& tidsJ,
    const std::unordered_map<int, Occurence>& occurencesJ) {
    
    int size = static_cast<int>(itemsetJ.size());
    int hash = computeHash(itemsetJ);
    
    auto& sizeMap = store[size];
    auto& classList = sizeMap[hash];
    
    for (auto& eclass : classList) {
        if (eclass.itemsetJ == itemsetJ) {
            eclass.rules.push_back(leftRule);
            return;
        }
    }
    
    std::vector<int> sortedTidsJ = tidsJ;
    std::sort(sortedTidsJ.begin(), sortedTidsJ.end());
    sortedTidsJ.erase(std::unique(sortedTidsJ.begin(), sortedTidsJ.end()), sortedTidsJ.end());
    
    LeftEquivalenceClass newClass(itemsetJ, sortedTidsJ, occurencesJ);
    newClass.rules.push_back(leftRule);
    classList.push_back(std::move(newClass));
}

int ExpandLeftStore::computeHash(const std::vector<int>& vec) const {
    int hash = 1;
    for (int item : vec) {
        hash = 31 * hash + item;
    }
    return hash;
}