#include "maxfem.h"

#include <algorithm>

#include "core/config/names.h"
#include "core/config/option.h"
#include "core/util/timed_invoke.h"

namespace algos::maxfem {

MaxFEM::MaxFEM() {
    RegisterOption(config::Option{
            &min_support_,
            config::names::kMinimumSupport,
            "Minimum support (count)",
            1ul,
    });
    RegisterOption(config::Option{
            &window_length_,
            config::names::kWindowSize,
            "Window size",
            5ul,
    });
}

void MaxFEM::MakeExecuteOptsAvailable() {
    Algorithm::MakeExecuteOptsAvailable();
    MakeOptionsAvailable({
            config::names::kMinimumSupport,
            config::names::kWindowSize,
    });
}

void MaxFEM::ResetState() {}

unsigned long long MaxFEM::ExecuteInternal() {
    return util::TimedInvoke(&MaxFEM::FindFrequentEpisodes, this);
}

void MaxFEM::FindFrequentEpisodes() {
    RemoveInfrequentEvents();
    auto parallel_episodes = FindFrequentParallelEpisodes();
    FindFrequentCompositeEpisodes(parallel_episodes);
}

void MaxFEM::RemoveInfrequentEvents() {
    std::map<model::Event, size_t> events_supports = GetEventsSupports();
    model::Event new_events_num = model::kStartEvent;
    reverse_mapping_.resize(new_events_num);

    for (auto const& [event, support] : events_supports) {
        if (support >= min_support_) {
            mapping_[event] = new_events_num;
            reverse_mapping_.push_back(event);
            new_events_num++;
        }
    }
    events_num_ = new_events_num;

    for (auto& event_set : *event_sequence_) {
        event_set.MapEventsAndRemoveInfrequent(mapping_);
    }
}

std::map<model::Event, size_t> MaxFEM::GetEventsSupports() const {
    std::map<model::Event, size_t> supports;
    for (auto const& event_set : *event_sequence_) {
        for (model::Event const event : event_set) {
            supports[event] += 1;
        }
    }
    return supports;
}

std::vector<ParallelEpisode> MaxFEM::FindFrequentParallelEpisodes() const {
    auto events_loc_lists = BuildEventsLocationLists();
    std::vector<ParallelEpisode> seeds =
            ParallelEpisode::BuildParallelEpisodesWithEvents(events_loc_lists, events_num_);
    std::vector<ParallelEpisode> parallel_episodes;

    for (auto const& seed : seeds) {
        FindFrequentParallelEpisodesRecursive(seed, events_loc_lists, parallel_episodes);
    }

    parallel_episodes.insert(parallel_episodes.end(), std::make_move_iterator(seeds.begin()),
                             std::make_move_iterator(seeds.end()));

    return parallel_episodes;
}

std::vector<std::shared_ptr<LocationList>> MaxFEM::BuildEventsLocationLists() const {
    std::vector<std::shared_ptr<LocationList>> location_lists(events_num_);
    std::generate(location_lists.begin(), location_lists.end(),
                  []() { return std::make_shared<LocationList>(); });

    for (auto const& event_set : *event_sequence_) {
        for (model::Event const event : event_set) {
            location_lists[event]->PushBack(event_set.GetTimestamp());
        }
    }
    return location_lists;
}

void MaxFEM::FindFrequentParallelEpisodesRecursive(
        ParallelEpisode const& current_episode,
        std::vector<std::shared_ptr<LocationList>> const& events_loc_lists,
        std::vector<ParallelEpisode>& results) const {
    for (model::Event event = current_episode.GetLastEvent() + 1; event < events_num_; ++event) {
        ParallelEpisode new_episode =
                current_episode.ParallelExtension(event, *events_loc_lists[event]);
        if (new_episode.GetSupport() >= min_support_) {
            FindFrequentParallelEpisodesRecursive(new_episode, events_loc_lists, results);
            results.push_back(std::move(new_episode));
        }
    }
}

void MaxFEM::FindFrequentCompositeEpisodes(std::vector<ParallelEpisode> const& parallel_episodes) {
    for (size_t index = 0; index < parallel_episodes.size(); ++index) {
        auto bound_list = BoundList(parallel_episodes[index]);
        auto episode = CompositeEpisode({parallel_episodes[index].GetEventSetPtr()},
                                        bound_list.GetSupport());
        bool has_extension =
                FindFrequentCompositeEpisodesRecursive(episode, bound_list, parallel_episodes);
        if (!has_extension) {
            max_episodes_collection_.Add(episode);
        }
    }

    max_frequent_episodes_ =
            max_episodes_collection_.GetResult(reverse_mapping_, parallel_episodes);
}

bool MaxFEM::FindFrequentCompositeEpisodesRecursive(
        CompositeEpisode& episode, BoundList const& bound_list,
        std::vector<ParallelEpisode> const& seed_episodes) {
    bool found_frequent_extension = false;

    for (auto const& parallel_episode : seed_episodes) {
        auto extended_bound_list =
                bound_list.Extend(parallel_episode.GetLocationList(), min_support_, window_length_);

        if (extended_bound_list) {
            found_frequent_extension = true;
            episode.Extend(parallel_episode, extended_bound_list->GetSupport());

            bool has_extension = FindFrequentCompositeEpisodesRecursive(
                    episode, *extended_bound_list, seed_episodes);
            if (!has_extension) {
                max_episodes_collection_.Add(episode);
            }

            episode.Shorten(bound_list.GetSupport());
        }
    }

    return found_frequent_extension;
}

}  // namespace algos::maxfem
