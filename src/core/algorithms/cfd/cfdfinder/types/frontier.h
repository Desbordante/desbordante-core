#pragma once

#include <queue>
#include <unordered_set>

#include "algorithms/cfd/cfdfinder/model/pattern/pattern.h"

namespace algos::cfdfinder {

class Frontier {
private:
    std::priority_queue<Pattern> sorted_index_;
    std::unordered_set<Entries> search_index_;

public:
    Frontier() = default;

    void Emplace(Pattern&& pattern) {
        search_index_.emplace(pattern.GetEntries());
        sorted_index_.emplace(std::move(pattern));
    }

    Pattern Poll() {
        auto pattern(sorted_index_.top());
        sorted_index_.pop();
        search_index_.erase(pattern.GetEntries());
        return pattern;
    }

    bool Contains(Pattern const& pattern) const {
        return search_index_.contains(pattern.GetEntries());
    }

    bool Empty() const {
        return sorted_index_.empty();
    }

    void Swap(Frontier& other) {
        sorted_index_.swap(other.sorted_index_);
        search_index_.swap(other.search_index_);
    }
};
}  // namespace algos::cfdfinder
