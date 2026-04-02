#pragma once
#include <vector>
#include <unordered_map>
#include <list>
#include "Occurence.hpp"
#include "RightRule.hpp"

class RightEquivalenceClass {
public:
    std::vector<int> itemsetI;                              
    std::vector<int> tidsI;                                   
    std::unordered_map<int, Occurence> occurencesI;
    std::list<RightRule> rules;                             
    
    RightEquivalenceClass(std::vector<int> itemsetI, std::vector<int> tidsI, std::unordered_map<int, Occurence> occurencesI);
    
    std::string toString() const;
    
    bool equals(const RightEquivalenceClass& other) const;
    
    bool operator==(const RightEquivalenceClass& other) const;
};