#include "LeftRule.hpp"
#include <sstream>
#include <string>
#include <algorithm>

LeftRule::LeftRule(std::vector<int> itemsetI, std::vector<int> tidsI, std::vector<int> tidsIJ): 
    itemsetI(std::move(itemsetI)), tidsI(std::move(tidsI)), tidsIJ(std::move(tidsIJ)){
        std::sort(this->tidsI.begin(), this->tidsI.end());
        this->tidsI.erase(std::unique(this->tidsI.begin(), this->tidsI.end()), this->tidsI.end());
        
        std::sort(this->tidsIJ.begin(), this->tidsIJ.end());
        this->tidsIJ.erase(std::unique(this->tidsIJ.begin(), this->tidsIJ.end()), this->tidsIJ.end());
}

std::string LeftRule::toString() const {
    std::stringstream ss;
    
    ss << "[";
    for (size_t i = 0; i < itemsetI.size(); i++) {
        ss << itemsetI[i];
        if (i < itemsetI.size() - 1) {
            ss << ", ";
        }
    }
    ss << "] ==> \"...\"";
    
    return ss.str();
}