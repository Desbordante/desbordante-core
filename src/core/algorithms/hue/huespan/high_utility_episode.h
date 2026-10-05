#pragma once

#include <iostream>
#include <utility>
#include <vector>

namespace algos::huespan {
class HighUtilityEpisode {
private:
    std::vector<std::vector<int>> event_sets_;
    int utility_;

public:
    HighUtilityEpisode() = default;

    HighUtilityEpisode(std::vector<std::vector<int>> event_sets, int utility)
        : event_sets_(std::move(event_sets)), utility_(utility) {}
    

    int GetUtility() const {
        return utility_;
    }

    std::vector<std::vector<int>> const& GetEvents() const {
        return event_sets_;
    }

    int GetSize() const {
        return event_sets_.size();
    }

    void Print(){
        for(int i = 0; i < event_sets_.size(); ++i) {
            std::cout << "[ ";
            for(int j = 0; j < event_sets_[i].size(); ++j) {
                std::cout << event_sets_[i][j] << ' ';
            }
            std::cout << "] ";
        }
    }
};
}  // namespace algos::huespan