#include "RightEquivalenceClass.hpp"
#include <sstream>
#include <string>
#include <algorithm>

RightEquivalenceClass::RightEquivalenceClass(std::vector<int> itemsetI, std::vector<int> tidsI, std::unordered_map<int, Occurence> occurencesI): 
    itemsetI(std::move(itemsetI)), tidsI(std::move(tidsI)), occurencesI(std::move(occurencesI)) {
    std::sort(this->tidsI.begin(), this->tidsI.end());
    this->tidsI.erase(std::unique(this->tidsI.begin(), this->tidsI.end()), this->tidsI.end());
}

std::string RightEquivalenceClass::toString() const {
    std::stringstream ss;
    
    ss << "[";
    for (size_t i = 0; i < itemsetI.size(); i++) {
        ss << itemsetI[i];
        if (i < itemsetI.size() - 1) {
            ss << ", ";
        }
    }
    ss << "] ==> EQ";
    
    return ss.str();
}

bool RightEquivalenceClass::equals(const RightEquivalenceClass& other) const {
    return itemsetI == other.itemsetI;
}

bool RightEquivalenceClass::operator==(const RightEquivalenceClass& other) const {
    return equals(other);
}