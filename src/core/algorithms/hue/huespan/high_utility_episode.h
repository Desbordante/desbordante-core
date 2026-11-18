#pragma once

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

    bool operator==(HighUtilityEpisode const& other) const {
        return utility_ == other.utility_ && event_sets_ == other.event_sets_;
    }
};
}  // namespace algos::huespan