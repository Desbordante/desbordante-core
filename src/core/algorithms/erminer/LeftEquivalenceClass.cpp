#include "LeftEquivalenceClass.hpp"
#include <sstream>
#include <string>
#include <algorithm>

LeftEquivalenceClass::LeftEquivalenceClass(std::vector<int> itemsetJ, std::vector<int> tidsJ, std::unordered_map<int, Occurence> occurencesJ): 
    itemsetJ(std::move(itemsetJ)), tidsJ(std::move(tidsJ)), occurencesJ(std::move(occurencesJ)) {
    std::sort(this->tidsJ.begin(), this->tidsJ.end());
    this->tidsJ.erase(std::unique(this->tidsJ.begin(), this->tidsJ.end()), this->tidsJ.end());
}

std::string LeftEquivalenceClass::toString() const {
    std::stringstream ss;
    
    ss << "EQ:[";
    for (size_t i = 0; i < itemsetJ.size(); i++) {
        ss << itemsetJ[i];
        if (i < itemsetJ.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    
    return ss.str();
}

bool LeftEquivalenceClass::equals(const LeftEquivalenceClass& other) const {
    return itemsetJ == other.itemsetJ;
}
bool LeftEquivalenceClass::operator==(const LeftEquivalenceClass& other) const {
    return equals(other);
}