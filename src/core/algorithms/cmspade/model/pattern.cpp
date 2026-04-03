#include "pattern.h"

#include <sstream>

namespace algos::cmspade{
void Pattern::Add(ItemAbstractionPair pair) {
    elements_.emplace_back(std::move(pair));
}

const ItemAbstractionPair& Pattern::GetLastElement() const {
    return elements_.back();
}

const ItemAbstractionPair& Pattern::GetPenultimateElement() const {
    return elements_[elements_.size() - 2];
}

bool Pattern::Equals(const Pattern& other) const {
    if (elements_.size() != other.elements_.size()) {
        return false;
    }
    for (std::size_t i = 0; i < elements_.size(); i++) {
        if (!elements_[i].Equals(other.elements_[i])) {
            return false;
        }
    }
    return true;
}

int Pattern::CompareTo(const Pattern& other) const {
    if (elements_.empty() && other.elements_.empty()) return 0;
    if (elements_.empty()) return -1;
    if (other.elements_.empty()) return 1;
    return elements_.back().CompareTo(other.elements_.back());
}

Pattern Pattern::Clone() const {
    Pattern new_pattern;
    new_pattern.elements_ = elements_;
    new_pattern.appearing_ = appearing_;
    return new_pattern;
}

bool Pattern::IsPrefix(const Pattern& other) const {
    if (other.Size() <= Size()) {
        return false;
    }
    for (std::size_t i = 0; i < elements_.size(); i++) {
        if (!elements_[i].Equals(other.elements_[i])) {
            return false;
        }
    }
    return true;
}

std::string Pattern::ToString() const {
    if (elements_.empty()) {
        return "{}";
    }

    std::stringstream ss;
    ss << "{ ";
    
    std::vector<ItemAbstractionPair> current_itemset;
    
    for (size_t idx = 0; idx < elements_.size(); ++idx) {
        const auto& pair = elements_[idx];
        
        if (pair.HaveEqualRelation()) {
            current_itemset.push_back(pair);
        } 
        else {
            if (!current_itemset.empty()) {
                ss << "[";
                for (size_t i = 0; i < current_itemset.size(); ++i) {
                    if (i > 0) ss << " ";
                    ss << current_itemset[i].GetItem().GetId();
                }
                ss << "] ";
                current_itemset.clear();
            }
            current_itemset.push_back(pair);
        }
    }
    
    if (!current_itemset.empty()) {
        ss << "[";
        for (size_t i = 0; i < current_itemset.size(); ++i) {
            if (i > 0) ss << " ";
            ss << current_itemset[i].GetItem().GetId();
        }
        ss << "]";
    }
    
    ss << " } (support: " << GetSupport() << ")";
    return ss.str();
}
} // namespace algos::cmspade