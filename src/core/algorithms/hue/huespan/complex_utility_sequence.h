#pragma once

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

#include "minimal_occurrences.h"

namespace algos::huespan {

class TimepointEvents {
private:
    std::vector<std::pair<int, int>> events_utility_pairs_;
    int total_utility_ = 0;

public:
    void AddEvent(int event, int utility) {
        events_utility_pairs_.push_back({event, utility});
        total_utility_ += utility;
    };

    void SetTotalUtility(int new_total_utility) {
        total_utility_ = new_total_utility;
    }

    int GetTotalUtility() const {
        return total_utility_;
    }

    std::vector<std::pair<int, int>>& GetEventsUtilityPairs() {
        return events_utility_pairs_;
    }

    std::vector<std::pair<int, int>> const& GetEventsUtilityPairs() const {
        return events_utility_pairs_;
    }
};

class ComplexSequence {
private:
    std::unordered_map<int, TimepointEvents> sequence_;
    size_t last_timestamp_ = 1;

public:
    ComplexSequence() = default;

    void AddEvent(int timepoint, int event, int utility) {
        if (!sequence_.contains(timepoint)) {
            last_timestamp_ = std::max(last_timestamp_, static_cast<size_t>(timepoint));
        }
        sequence_[timepoint].AddEvent(event, utility);
    }

    std::vector<std::pair<int, int>> GetEventsByTimepoint(int timepoint) const {
        auto it = sequence_.find(timepoint);
        if (it != sequence_.end()) {
            return it->second.GetEventsUtilityPairs();
        }
        return {};
    }

    size_t GetLastTimepoint() const {
        return last_timestamp_;
    }

    void PruneSingleEvents(int max_duration, double min_utility_absolute,
                           std::unordered_map<int, EventMinimalOccurrences>& events_mos);
    int GetUtilityOfDuration(int start_timepoint, int end_timepoint);
    int GetRemainingUtility(int timepoint, int event) const;

    int GetEventsUtilityByTimepoint(std::vector<int> const& eventset, int timepoint) const;
    int GetMaximalUtility(std::vector<std::vector<int>> const& episode, int start_timepoint,
                          int end_timepoint);
    std::vector<std::pair<int, int>> GetExtensionEvents(int timepoint, int last_event) const;
};
}  // namespace algos::huespan