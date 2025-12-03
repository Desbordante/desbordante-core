#include "maxfem.h"

#include <algorithm>

#include "util/timed_invoke.h"

namespace algos::maxfem {

void MaxFEM::ResetState() {}

unsigned long long MaxFEM::ExecuteInternal() {
    return util::TimedInvoke(&MaxFEM::FindFrequentEpisodes, this);
}

void MaxFEM::FindFrequentEpisodes() {
    RemoveInfrequentEvents();
    auto parallel_episodes = FindFrequentParallelEpisodes();
}

std::vector<size_t> MaxFEM::GetEventsSupports() const {
    std::vector<size_t> supports(events_num_, 0);
    for (auto const& event_set : *event_sequence_) {
        for (model::Event const event : event_set) {
            supports[event] += 1;
        }
    }
    return supports;
}

void MaxFEM::RemoveInfrequentEvents() {
    std::vector<size_t> events_supports = GetEventsSupports();

    std::vector<model::Event> mapping(events_num_, model::kInvalidEvent);
    model::Event new_events_num = model::kStartEvent;

    for (model::Event event = model::kStartEvent; event < events_num_; ++event) {
        if (events_supports[event] >= min_support_) {
            mapping[event] = new_events_num;
            events_supports[new_events_num] = events_supports[event];
            new_events_num++;
        }
    }
    events_supports.resize(new_events_num);
    events_num_ = new_events_num;

    for (auto& event_set : *event_sequence_) {
        event_set.mapEvents(mapping);
    }
}

std::vector<ParallelEpisode> MaxFEM::FindFrequentParallelEpisodes() const {
    auto events_loc_lists = BuildEventsLocationLists();
    std::vector<ParallelEpisode> parallel_episodes =
        ParallelEpisode::BuildParallelEpisodes(*event_sequence_, events_loc_lists, events_num_);

    for (auto const& parallel_episode : parallel_episodes) {
        for (model::Event event = model::kStartEvent; event < events_num_; ++event) {
            ParallelEpisode new_episode = parallel_episode.ParallelExtension(event, *events_loc_lists[event]);
        }
    }

    return parallel_episodes;
}

std::vector<std::shared_ptr<LocationList>> MaxFEM::BuildEventsLocationLists() const {
    std::vector<std::shared_ptr<LocationList>> location_lists(events_num_);
    std::generate(location_lists.begin(), location_lists.end(), []() {
        return std::make_shared<LocationList>();
    });

    for (size_t index = 0; index < event_sequence_->Size(); ++index) {
        for (model::Event const event : event_sequence_->At(index)) {
            location_lists[event]->PushBack(index);
        }
    }
    return location_lists;
}

}  // namespace algos::maxfem
