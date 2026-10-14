#include "max_episodes_collection.h"

#include <algorithm>
#include <cassert>

namespace algos::maxfem {

void MaxEpisodesCollection::SimpleAdd(CompositeEpisode const& episode) {
    size_t length = episode.GetLength();
    while (length >= max_episodes_.size()) {
        max_episodes_.emplace_back();
    }
    max_episodes_[length].insert(episode);
}

void MaxEpisodesCollection::BatchFill(std::vector<MaxEpisodesCollection>& collections) {
    assert(max_episodes_.size() == 0);

    size_t global_max_len = 0;
    for (auto const& collection : collections) {
        if (!collection.max_episodes_.empty()) {
            global_max_len = std::max(global_max_len, collection.max_episodes_.size() - 1);
        }
    }

    while (global_max_len >= max_episodes_.size()) {
        max_episodes_.emplace_back();
    }

    std::vector<EpisodeSet::node_type> bucket_buffer;

    for (int len = static_cast<int>(global_max_len); len >= 0; --len) {
        size_t length = static_cast<size_t>(len);
        for (auto& collection : collections) {
            if (length >= collection.max_episodes_.size()) {
                continue;
            }

            auto& source_set = collection.max_episodes_[length];
            auto iter = source_set.begin();
            while (iter != source_set.end()) {
                bucket_buffer.push_back(source_set.extract(iter++));
            }
        }

        if (bucket_buffer.empty()) {
            continue;
        }

        std::sort(bucket_buffer.begin(), bucket_buffer.end(),
                  [](EpisodeSet::node_type const& a, EpisodeSet::node_type const& b) {
                      return a.value().GetEventsSum() > b.value().GetEventsSum();
                  });

        for (auto& node : bucket_buffer) {
            if (CheckForSuperEpisode(node.value(), length)) {
                continue;
            }
            max_episodes_[length].insert(std::move(node));
        }

        bucket_buffer.clear();
    }

    for (auto& collection : collections) {
        collection.max_episodes_.clear();
    }
}

std::vector<CompositeEpisode::RawEpisode> MaxEpisodesCollection::GetResult(
        std::vector<model::Event> const& mapping,
        std::vector<ParallelEpisode> const& parallel_episodes) {
    std::vector<CompositeEpisode::RawEpisode> result;

    size_t total_count = 0;
    for (auto const& s : max_episodes_) {
        total_count += s.size();
    }
    result.reserve(total_count);

    for (auto const& parallel_episode : parallel_episodes) {
        parallel_episode.GetEventSetPtr()->MapEvents(mapping);
    }

    for (auto& episodes_set : max_episodes_) {
        for (auto const& ep : episodes_set) {
            result.push_back(ep.GetRaw());
        }
        episodes_set.clear();
    }

    max_episodes_.clear();
    return result;
}

bool MaxEpisodesCollection::CheckForSuperEpisode(CompositeEpisode const& episode,
                                                 size_t length) const {
    for (size_t index = max_episodes_.size(); index-- > length;) {
        auto& max_episodes_set = max_episodes_[index];
        for (auto iter = max_episodes_set.rbegin(); iter != max_episodes_set.rend(); ++iter) {
            auto const& max_episode = *iter;

            if (max_episode.GetEventsSum() < episode.GetEventsSum()) {
                break;
            }

            if (episode.GetEvenEventsSum() <= max_episode.GetEvenEventsSum() &&
                episode.GetOddEventsSum() <= max_episode.GetOddEventsSum() &&
                max_episode.StrictlyContains(episode)) {
                return true;
            }
        }
    }

    return false;
}

}  // namespace algos::maxfem
