#include "Sequence.hpp"
#include <sstream>
#include <memory>

Sequence::Sequence(int id) : id(id) {}

void Sequence::addItemset(const std::vector<int>& itemset) {
    itemsets.push_back(itemset);
}

int Sequence::getId() const {
    return id;
}

const std::vector<std::vector<int>>& Sequence::getItemsets() const {
    return itemsets;
}

const std::vector<int>& Sequence::get(int index) const {
    return itemsets[index];
}

int Sequence::size() const {
    return static_cast<int>(itemsets.size());
}

std::string Sequence::toString() const {
    std::stringstream ss;
    
    for (const auto& itemset : itemsets) {
        ss << '(';
        for (size_t i = 0; i < itemset.size(); i++) {
            ss << itemset[i];
            if (i < itemset.size() - 1) {
                ss << ' ';
            }
        }
        ss << ')';
    }
    ss << "    ";
    
    return ss.str();
}

std::unique_ptr<Sequence> Sequence::cloneSequenceMinusItems(
    const std::unordered_map<int, std::unordered_set<int>>& mapSequenceID,
    double relativeMinSup) const {
    
    auto newSequence = std::make_unique<Sequence>(id);
    
    for (const auto& itemset : itemsets) {
        std::vector<int> newItemset = cloneItemsetMinusItems(itemset, mapSequenceID, relativeMinSup);
        if (!newItemset.empty()) {
            newSequence->addItemset(newItemset);
        }
    }
    
    return newSequence;
}

std::unique_ptr<Sequence> Sequence::cloneSequenceMinusItems(
    double relativeMinSup,
    const std::unordered_map<int, std::unordered_set<Sequence*>>& mapSequenceID) const {
    
    auto newSequence = std::make_unique<Sequence>(id);
    
    for (const auto& itemset : itemsets) {
        std::vector<int> newItemset = cloneItemsetMinusItems(relativeMinSup, itemset, mapSequenceID);
        if (!newItemset.empty()) {
            newSequence->addItemset(newItemset);
        }
    }
    
    return newSequence;
}

std::vector<int> Sequence::cloneItemsetMinusItems(
    const std::vector<int>& itemset,
    const std::unordered_map<int, std::unordered_set<int>>& mapSequenceID,
    double minSupportAbsolute) const {
    
    std::vector<int> newItemset;
    
    for (int item : itemset) {
        auto it = mapSequenceID.find(item);
        if (it != mapSequenceID.end() && it->second.size() >= minSupportAbsolute) {
            newItemset.push_back(item);
        }
    }
    
    return newItemset;
}

std::vector<int> Sequence::cloneItemsetMinusItems(
    double relativeMinsup,
    const std::vector<int>& itemset,
    const std::unordered_map<int, std::unordered_set<Sequence*>>& mapSequenceID) const {
    
    std::vector<int> newItemset;
    
    for (int item : itemset) {
        auto it = mapSequenceID.find(item);
        if (it != mapSequenceID.end() && it->second.size() >= relativeMinsup) {
            newItemset.push_back(item);
        }
    }
    
    return newItemset;
}