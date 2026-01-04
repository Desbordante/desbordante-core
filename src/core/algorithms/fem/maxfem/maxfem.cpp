#include "maxfem.h"

#include <algorithm>

#include "core/util/timed_invoke.h"

namespace algos::maxfem {

void MaxFEM::ResetState() {}

unsigned long long MaxFEM::ExecuteInternal() {
    return util::TimedInvoke(&MaxFEM::FindFrequentEpisodes, this);
}

void MaxFEM::FindFrequentEpisodes() {
    RemoveInfrequentEvents();
    auto parallel_episodes = FindFrequentParallelEpisodes();
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
        event_set.MapEvents(mapping);
    }
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

std::vector<ParallelEpisode> MaxFEM::FindFrequentParallelEpisodes() const {
    auto events_loc_lists = BuildEventsLocationLists();
    std::vector<ParallelEpisode> seeds = ParallelEpisode::BuildParallelEpisodesWithEvents(
            *event_sequence_, events_loc_lists, events_num_);
    std::vector<ParallelEpisode> all_episodes;

    for (auto const& seed : seeds) {
        FindFrequentEpisodesRecursive(seed, events_loc_lists, all_episodes);
    }

    all_episodes.insert(all_episodes.end(), std::make_move_iterator(seeds.begin()),
                        std::make_move_iterator(seeds.end()));

    return all_episodes;
}

std::vector<std::shared_ptr<LocationList>> MaxFEM::BuildEventsLocationLists() const {
    std::vector<std::shared_ptr<LocationList>> location_lists(events_num_);
    std::generate(location_lists.begin(), location_lists.end(),
                  []() { return std::make_shared<LocationList>(); });

    for (size_t index = 0; index < event_sequence_->Size(); ++index) {
        for (model::Event const event : event_sequence_->At(index)) {
            location_lists[event]->PushBack(index);
        }
    }
    return location_lists;
}

void MaxFEM::FindFrequentEpisodesRecursive(
        ParallelEpisode const& current_episode,
        std::vector<std::shared_ptr<LocationList>> const& events_loc_lists,
        std::vector<ParallelEpisode>& results) const {
    for (model::Event event = current_episode.GetLastEvent() + 1; event < events_num_; ++event) {
        ParallelEpisode new_episode =
                current_episode.ParallelExtension(event, *events_loc_lists[event]);
        if (new_episode.GetSupport() >= min_support_) {
            FindFrequentEpisodesRecursive(new_episode, events_loc_lists, results);
            results.push_back(std::move(new_episode));
        }
    }
}

}  // namespace algos::maxfem
