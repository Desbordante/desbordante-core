#pragma once
#include <string>
#include <vector>

class LeftRule {
public:
    std::vector<int> itemsetI;
    std::vector<int> tidsI;
    std::vector<int> tidsIJ;
    
    LeftRule(std::vector<int> itemsetI, 
             std::vector<int> tidsI,
             std::vector<int> tidsIJ);
    
    std::string toString() const;
};