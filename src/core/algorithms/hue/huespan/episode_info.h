#pragma once

#include <vector>

namespace algos::huespan {
class EventMOs {
private:
    std::vector<int> minimal_occurencies_;

    std::vector<int> utilities_;

public:
    EventMOs() = default;

    EventMOs(std::vector<int> minimal_occurences, std::vector<int> utilities)
        : minimal_occurencies_(std::move(minimal_occurences)), utilities_(std::move(utilities)) {}

    void Add(int minimal_occurency, int utility) {
        minimal_occurencies_.push_back(minimal_occurency);
        utilities_.push_back(utility);
    }

    std::vector<int> const& GetMinOccurencies() {
        return minimal_occurencies_;
    }

    std::vector<int> const& GetUtilities() {
        return utilities_;
    }
};

struct EpisodeMinimalOccurences {
    std::vector<int> start_timepoints;
    std::vector<int> end_timepoints;
    std::vector<int> utilities;
    std::vector<int> prev_utilities;
};
}  // namespace algos::huespan