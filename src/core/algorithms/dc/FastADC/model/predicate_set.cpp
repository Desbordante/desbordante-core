#include "predicate_set.h"

namespace algos::fastadc {

bool PredicateSet::Add(PredicatePtr predicate) {
    auto index = PredicateIndexProvider::GetInstance()->GetIndex(predicate);
    if (index >= bitset_.size()) {
        // Resize the bitset to accommodate the index
        // Double the current size, or grow just beyond the requested index (whichever is bigger)
        bitset_.resize(std::max(2 * bitset_.size(), index + 1));
    }
    bool just_added = !bitset_.test(index);
    bitset_.set(index);
    return just_added;
}

bool PredicateSet::Contains(PredicatePtr predicate) const {
    auto index = PredicateIndexProvider::GetInstance()->GetIndex(predicate);
    return index < bitset_.size() ? bitset_.test(index) : false;
}

// Implementation of the GetInvTS method (caching the result)
PredicateSet PredicateSet::GetInvTS() const {
    if (inv_set_TS_) {
        return *inv_set_TS_;
    }

    PredicateSet result;
    for (PredicatePtr predicate : *this) {
        result.Add(predicate->GetInvTS());
    }

    inv_set_TS_ = std::make_unique<PredicateSet>(result);
    return result;
}

PredicateSet::Iterator PredicateSet::begin() const {
    return Iterator(this, 0);
}

PredicateSet::Iterator PredicateSet::end() const {
    return Iterator(this, bitset_.size());
}

std::string PredicateSet::ToString() const {
    std::string result = "{ ";
    for (PredicatePtr predicate : *this) {
        result += predicate->ToString() + " ";
    }
    result += "}";
    return result;
}

}  // namespace algos::fastadc
