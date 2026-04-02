#pragma once
#include <unordered_map>
#include <list>
#include <vector>
#include "Occurence.hpp"
#include "LeftRule.hpp"
#include "LeftEquivalenceClass.hpp"

class ExpandLeftStore {
public:
    std::unordered_map<int, std::unordered_map<int, std::list<LeftEquivalenceClass>>> store;
    
    ExpandLeftStore() = default;
    
    void registerRule(const LeftRule& leftRule,
        const std::vector<int>& itemsetJ,const std::vector<int>& tidsJ,
        const std::unordered_map<int, Occurence>& occurencesJ);
        
private:
    int computeHash(const std::vector<int>& vec) const;
};