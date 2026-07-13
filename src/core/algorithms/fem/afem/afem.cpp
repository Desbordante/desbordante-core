#include "afem.h"

#include <algorithm>
#include <unordered_map>

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include "core/algorithms/fem/afem/composite_episode_miner.h"
#include "core/config/names.h"
#include "core/config/option.h"
#include "core/config/thread_number/option.h"
#include "core/util/logger.h"

namespace algos::afem {

AFEM::AFEM() {
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
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
    RegisterOption(config::Option{
            &tasks_num_multiplier_,
            config::names::kTasksNumMultiplier,
            "Ratio of tasks number to threads number",
            3.0,
    });
}

void AFEM::MakeExecuteOptsAvailable() {
    Algorithm::MakeExecuteOptsAvailable();
    MakeOptionsAvailable({
            config::names::kMinimumSupport,
            config::names::kWindowSize,
            config::names::kThreads,
            config::names::kTasksNumMultiplier,
    });
}

void AFEM::ResetState() {}

void AFEM::ExecuteInternal() {
    LOG_DEBUG("Min support: {}. Window length: {}", min_support_, window_length_);
    LOG_DEBUG("Threads num: {}", threads_num_);
    LOG_DEBUG("Sequence length: {}", event_sequence_->Size());
    RemoveInfrequentEvents();
    auto parallel_episodes = FindFrequentParallelEpisodes();
    LOG_DEBUG("Frequent parallel episodes number: {}", parallel_episodes.size());
    FindFrequentCompositeEpisodes(parallel_episodes);
}

void AFEM::RemoveInfrequentEvents() {
    std::map<model::Event, size_t> events_supports = GetEventsSupports();
    model::Event new_events_num = model::kStartEvent;
    reverse_mapping_.resize(new_events_num);

    std::unordered_map<model::Event, model::Event> mapping;
    for (auto const& [event, support] : events_supports) {
        if (support >= min_support_) {
            mapping[event] = new_events_num;
            reverse_mapping_.push_back(event);
            new_events_num++;
        }
    }
    events_num_ = new_events_num;
    LOG_DEBUG("Frequent events number: {}", events_num_);

    for (auto& event_set : *event_sequence_) {
        event_set.MapEventsAndRemoveInfrequent(mapping);
    }
}

std::map<model::Event, size_t> AFEM::GetEventsSupports() const {
    std::map<model::Event, size_t> supports;
    for (auto const& event_set : *event_sequence_) {
        for (model::Event const event : event_set) {
            supports[event] += 1;
        }
    }
    LOG_DEBUG("Events number: {}", supports.size());
    return supports;
}

std::vector<maxfem::ParallelEpisode> AFEM::FindFrequentParallelEpisodes() const {
    auto events_loc_lists = BuildEventsLocationLists();
    std::vector<maxfem::ParallelEpisode> seeds =
            maxfem::ParallelEpisode::BuildParallelEpisodesWithEvents(events_loc_lists, events_num_);
    std::vector<maxfem::ParallelEpisode> parallel_episodes;

    for (auto const& seed : seeds) {
        FindFrequentParallelEpisodesRecursive(seed, events_loc_lists, parallel_episodes);
    }

    parallel_episodes.insert(parallel_episodes.end(), std::make_move_iterator(seeds.begin()),
                             std::make_move_iterator(seeds.end()));

    return parallel_episodes;
}

std::vector<std::shared_ptr<maxfem::LocationList>> AFEM::BuildEventsLocationLists() const {
    std::vector<std::shared_ptr<maxfem::LocationList>> location_lists(events_num_);
    std::generate(location_lists.begin(), location_lists.end(),
                  []() { return std::make_shared<maxfem::LocationList>(); });

    for (auto const& event_set : *event_sequence_) {
        for (model::Event const event : event_set) {
            location_lists[event]->PushBack(event_set.GetTimestamp());
        }
    }
    return location_lists;
}

void AFEM::FindFrequentParallelEpisodesRecursive(
        maxfem::ParallelEpisode const& current_episode,
        std::vector<std::shared_ptr<maxfem::LocationList>> const& events_loc_lists,
        std::vector<maxfem::ParallelEpisode>& results) const {
    for (model::Event event = current_episode.GetLastEvent() + 1; event < events_num_; ++event) {
        maxfem::ParallelEpisode new_episode =
                current_episode.ParallelExtension(event, *events_loc_lists[event]);
        if (new_episode.GetSupport() >= min_support_) {
            FindFrequentParallelEpisodesRecursive(new_episode, events_loc_lists, results);
            results.push_back(std::move(new_episode));
        }
    }
}

void AFEM::FindFrequentCompositeEpisodes(
        std::vector<maxfem::ParallelEpisode> const& parallel_episodes) {
    CompositeEpisodeMiner miner(min_support_, window_length_, threads_num_, tasks_num_multiplier_);
    auto raw_results = miner.Mine(parallel_episodes);

    for (auto const& parallel_episode : parallel_episodes) {
        parallel_episode.GetEventSetPtr()->MapEvents(reverse_mapping_);
    }

    for (auto& thread_results : raw_results) {
        for (auto& ep : thread_results) {
            frequent_episodes_.push_back(ep.GetRaw());
        }
    }
}

}  // namespace algos::afem
