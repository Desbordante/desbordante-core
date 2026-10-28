#pragma once

#include <vector>

namespace algos::huespan {
class EventMinimalOccurrences {
private:
    std::vector<int> minimal_occurrences_;

    std::vector<int> utilities_;

public:
    EventMinimalOccurrences() = default;

    EventMinimalOccurrences(std::vector<int> minimal_occurrences, std::vector<int> utilities)
        : minimal_occurrences_(std::move(minimal_occurrences)), utilities_(std::move(utilities)) {}

    void Add(int minimal_occurency, int utility) {
        minimal_occurrences_.push_back(minimal_occurency);
        utilities_.push_back(utility);
    }

    std::vector<int> const& GetOccurrences() {
        return minimal_occurrences_;
    }

    std::vector<int> const& GetUtilities() {
        return utilities_;
    }
};

struct EpisodeMinimalOccurrences {
    std::vector<int> start_timepoints;
    std::vector<int> end_timepoints;
    std::vector<int> utilities;
    std::vector<int> prev_utilities;
};
}  // namespace algos::huespan