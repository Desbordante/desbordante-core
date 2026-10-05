#pragma once

#include <vector>

namespace algos::huespan {
class FiniteStateMachine {
private:
    std::vector<std::vector<int>> fsa_;
    int pos_ = 0;
    int utility_ = 0;

public:
    FiniteStateMachine() = default;

    FiniteStateMachine(std::vector<std::vector<int>> const& episode, int first_eventset_utility)
        : fsa_(episode), utility_(first_eventset_utility) {
        // to skip the starting point
        Transit();
    }

    void Transit() {
        pos_++;
    }

    std::vector<int> const& WaitForEvents() const {
        return fsa_[pos_];
    }

    bool IsEnd() const {
        return pos_ == fsa_.size();
    }

    bool IsSame(FiniteStateMachine const& other) const {
        return pos_ == other.pos_;
    }

    int GetUtility() const {
        return utility_;
    }

    bool Scan(std::vector<std::pair<int, int>> const& pairs) {
        int eventset_utility = 0;
        int index = 0;
        int length = fsa_[pos_].size();
        for (auto const& pair : pairs) {
            if (pair.first == fsa_[pos_][index]) {
                index++;
                eventset_utility += pair.second;
            }
            if (index == length) {
                utility_ += eventset_utility;
                break;
            }
        }
        return index == length;
    }
};
}  // namespace algos::huespan