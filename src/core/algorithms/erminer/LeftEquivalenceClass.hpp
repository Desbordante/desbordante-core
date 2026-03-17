#pragma once
#include <vector>
#include <list>
#include <unordered_map>
#include "Occurence.hpp"
#include "LeftRule.hpp"

class LeftEquivalenceClass {
public:
    std::vector<int> itemsetJ;                              
    std::vector<int> tidsJ;                                   
    std::unordered_map<int, Occurence> occurencesJ;
    std::list<LeftRule> rules;
    
    LeftEquivalenceClass(std::vector<int> itemsetJ, std::vector<int> tidsJ, std::unordered_map<int, Occurence> occurencesJ);
    
    std::string toString() const;    
    bool equals(const LeftEquivalenceClass& other) const;
    bool operator==(const LeftEquivalenceClass& other) const;
};