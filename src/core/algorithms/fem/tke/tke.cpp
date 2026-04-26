#include "tke.h"

#include <algorithm>
#include <unordered_map>

#include "core/algorithms/fem/tke/composite_topk_miner.h"
#include "core/algorithms/fem/tke/parallel_topk_miner.h"
#include "core/config/names.h"
#include "core/config/option.h"
#include "core/config/thread_number/option.h"
#include "core/model/sequence/timed_event_set.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"

namespace algos::tke {

TKE::TKE() {
    RegisterOption(config::Option{
            &episodes_num_,
            config::names::kEpisodesNum,
            "Number of top frequent episodes (k)",
            10ul,
    });
    RegisterOption(config::Option{
            &window_length_,
            config::names::kWindowSize,
            "Window size",
            5ul,
    });
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
}

void TKE::MakeExecuteOptsAvailable() {
    Algorithm::MakeExecuteOptsAvailable();
    MakeOptionsAvailable({
            config::names::kEpisodesNum,
            config::names::kWindowSize,
            config::names::kThreads,
    });
}

void TKE::ResetState() {}

unsigned long long TKE::ExecuteInternal() {
    return util::TimedInvoke(&TKE::FindFrequentEpisodes, this);
}

void TKE::FindFrequentEpisodes() {
    LOG_DEBUG("Episodes num: {}. Window length: {}", episodes_num_, window_length_);
    LOG_DEBUG("Threads num: {}", threads_num_);
    LOG_DEBUG("Sequence length: {}", event_sequence_->Size());

    std::map<model::Event, size_t> const raw_supports = GetEventsSupports();

    std::vector<size_t> supports_vec;
    supports_vec.reserve(raw_supports.size());
    for (auto const& [_, s] : raw_supports) {
        supports_vec.push_back(s);
    }

    size_t events_minsup = 1;
    if (supports_vec.size() > episodes_num_) {
        auto const nth = supports_vec.end() - static_cast<std::ptrdiff_t>(episodes_num_);
        std::nth_element(supports_vec.begin(), nth, supports_vec.end());
        events_minsup = *nth;
    }

    RemoveInfrequentEvents(raw_supports, events_minsup);

    ParallelTopKMiner parallel_miner(events_num_, episodes_num_, BuildEventsLocationLists(), threads_num_);
    std::vector<ParallelEpisode> parallel_episodes = parallel_miner.Mine();
    LOG_DEBUG("Parallel episodes after top-k mining: {}", parallel_episodes.size());

    size_t const parallel_minsup = (parallel_episodes.size() >= episodes_num_)
                                           ? parallel_episodes.back().GetSupport()
                                           : 1;
    LOG_DEBUG("Initial minsup for composite phase (from parallel): {}", parallel_minsup);

    CompositeTopKMiner composite_miner(episodes_num_, window_length_, threads_num_);
    DecodeAndStoreResults(composite_miner.Mine(std::move(parallel_episodes), parallel_minsup));
}

std::map<model::Event, size_t> TKE::GetEventsSupports() const {
    std::map<model::Event, size_t> supports;
    for (model::TimedEventSet const& event_set : *event_sequence_) {
        for (model::Event const event : event_set) {
            supports[event] += 1;
        }
    }
    LOG_DEBUG("Distinct raw events: {}", supports.size());
    return supports;
}

void TKE::RemoveInfrequentEvents(std::map<model::Event, size_t> const& events_supports,
                                         size_t event_minsup) {
    model::Event new_events_num = model::kStartEvent;
    reverse_mapping_.clear();
    reverse_mapping_.resize(new_events_num);

    std::unordered_map<model::Event, model::Event> mapping;
    for (auto const& [event, support] : events_supports) {
        if (support >= event_minsup) {
            mapping[event] = new_events_num;
            reverse_mapping_.push_back(event);
            new_events_num++;
        }
    }
    events_num_ = new_events_num;
    LOG_DEBUG("Frequent events: {}", events_num_);

    for (model::TimedEventSet& event_set : *event_sequence_) {
        event_set.MapEventsAndRemoveInfrequent(mapping);
    }
}

std::vector<std::shared_ptr<LocationList>> TKE::BuildEventsLocationLists() const {
    std::vector<std::shared_ptr<LocationList>> location_lists(events_num_);
    std::generate(location_lists.begin(), location_lists.end(),
                  []() { return std::make_shared<LocationList>(); });

    for (model::TimedEventSet const& event_set : *event_sequence_) {
        model::Timestamp const ts = event_set.GetTimestamp();
        for (model::Event const event : event_set) {
            location_lists[event]->PushBack(ts);
        }
    }
    return location_lists;
}

void TKE::DecodeAndStoreResults(std::vector<CompositeEpisode>&& composites) {
    top_k_frequent_episodes_.clear();
    top_k_frequent_episodes_.reserve(composites.size());
    for (CompositeEpisode& ce : composites) {
        CompositeEpisode::RawEpisode raw = ce.GetRaw();
        for (std::vector<model::Event>& block : raw.first) {
            for (model::Event& ev : block) {
                ev = reverse_mapping_[ev];
            }
        }
        top_k_frequent_episodes_.push_back(std::move(raw));
    }
}

}  // namespace algos::tke
