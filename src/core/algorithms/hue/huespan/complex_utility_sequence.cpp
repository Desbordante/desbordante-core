#include <algorithm>

#include "complex_util_sequence.h"
#include "finite_state_machine.h"

namespace algos::huespan {
void ComplexSequence::PruneSingleEvents(
        int max_duration, double min_utility_absolute,
        std::unordered_map<int, EpisodeMinimalOccsUtilities>& EpisodesMinimalOccsUtilities) {
    std::unordered_map<int, int> events_awu;
    for (auto& [timepoint, events_utility_pairs] : sequence_) {
        int awu = GetUtilityOfDuration(timepoint - max_duration + 1, timepoint + max_duration - 1);
        for (auto const& pair : events_utility_pairs.GetEventsUtilityPairs()) {
            events_awu[pair.first] += awu;
        }
    }

    for (auto it = EpisodesMinimalOccsUtilities.begin();
         it != EpisodesMinimalOccsUtilities.end();) {
        auto awu_it = events_awu.find(it->first);

        if (awu_it == events_awu.end() || awu_it->second < min_utility_absolute) {
            it = EpisodesMinimalOccsUtilities.erase(it);
        } else {
            ++it;
        }
    }

    for (auto timepoint_it = sequence_.begin(); timepoint_it != sequence_.end();) {
        auto& events = timepoint_it->second;
        auto& pairs = events.GetEventsUtilityPairs();

        int removed_utility_sum = 0;

        for (int i = pairs.size() - 1; i >= 0; --i) {
            int event = pairs[i].first;
            if (EpisodesMinimalOccsUtilities.find(event) == EpisodesMinimalOccsUtilities.end()) {
                removed_utility_sum += pairs[i].second;
                pairs.erase(pairs.begin() + i);
            }
        }

        if (!pairs.empty()) {
            std::sort(pairs.begin(), pairs.end(),
                      [&](std::pair<int, int> const& a, std::pair<int, int> const& b) {
                          return events_awu[a.first] < events_awu[b.first];
                      });
            events.SetTotalUtility(events.GetTotalUtility() - removed_utility_sum);
            ++timepoint_it;
        } else {
            timepoint_it = sequence_.erase(timepoint_it);
        }
    }
}

int ComplexSequence::GetUtilityOfDuration(int start_timepoint, int end_timepoint) {
    if (start_timepoint > static_cast<int>(last_timestamp_)) return 0;
    if (end_timepoint > static_cast<int>(last_timestamp_)) end_timepoint = sequence_.size();

    int total_utility = 0;
    for (int timepoint = start_timepoint; timepoint <= end_timepoint; ++timepoint) {
        if (sequence_.contains(timepoint)) {
            total_utility += sequence_[timepoint].GetTotalUtility();
        }
    }
    return total_utility;
}

int ComplexSequence::GetRemainingUtility(int timepoint, int event) const {
    int utility = 0;
    auto const& pairs = sequence_.at(timepoint).GetEventsUtilityPairs();
    for (int i = pairs.size() - 1; i > 0; --i) {
        if (pairs[i].first != event) {
            utility += pairs[i].second;
        } else {
            break;
        }
    }
    return utility;
}

std::vector<std::pair<int, int>> ComplexSequence::GetExtentionEvents(int timepoint,
                                                                     int last_event) const {
    auto& pairs = sequence_.at(timepoint).GetEventsUtilityPairs();
    for (int i = 1; i < pairs.size(); ++i) {
        if (pairs[i - 1].first == last_event) {
            return std::vector<std::pair<int, int>>(pairs.begin() + i, pairs.end());
        }
    }
    return {};
}

int ComplexSequence::GetEventsUtilityByTimepoint(std::vector<int> const& eventset,
                                                 int timepoint) const {
    int utility = 0;
    int index = 0;
    auto const& pairs = sequence_.at(timepoint).GetEventsUtilityPairs();

    for (int i = 0; i < pairs.size(); ++i) {
        if (index == eventset.size()) break;

        if (pairs[i].first == eventset[index]) {
            index++;
            utility += pairs[i].second;
        }
    }
    return index == eventset.size() ? utility : 0;
}

int ComplexSequence::GetMaximallUtility(std::vector<std::vector<int>> episode, int start_timepoint,
                                        int end_timepoint) {
    int max_utility = 0;
    std::vector<FiniteStateMachine> state_machines;

    int first_eventset_utility = GetEventsUtilityByTimepoint(episode[0], start_timepoint);
    state_machines.emplace_back(episode, first_eventset_utility);

    for (int timepoint = start_timepoint + 1; timepoint <= end_timepoint; ++timepoint) {
        if (!sequence_.contains(timepoint)) continue;

        auto& pairs = sequence_[timepoint].GetEventsUtilityPairs();

        for (int i = state_machines.size() - 1; i >= 0; --i) {
            if (state_machines[i].Scan(pairs)) {
                state_machines[i].Transit();
                if (i == state_machines.size() - 1) {
                    state_machines.emplace_back(episode, first_eventset_utility);
                }
                if (state_machines[i].IsEnd()) {
                    max_utility = std::max(max_utility, state_machines[i].GetUtility());
                    state_machines.erase(state_machines.begin() + i);
                } else if (i >= 1 && state_machines[i].IsSame(state_machines[i - 1]) &&
                           !state_machines[i - 1].Scan(pairs)) {
                    if (state_machines[i].GetUtility() >= state_machines[i - 1].GetUtility()) {
                        state_machines.erase(state_machines.begin() + i - 1);
                        i--;
                    } else {
                        state_machines.erase(state_machines.begin() + i);
                    }
                }
            }
        }
    }
    return max_utility;
}
}  // namespace algos::huespan