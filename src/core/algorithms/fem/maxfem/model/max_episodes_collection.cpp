#include "max_episodes_collection.h"

#include <cassert>

namespace algos::maxfem {

void MaxEpisodesCollection::Add(CompositeEpisode const& episode) {
    size_t length = episode.GetLength();

    if (CheckForSuperEpisode(episode, length)) {
        return;
    }
    while (length >= max_episodes_.size()) {
        max_episodes_.emplace_back();
    }
    RemoveSubEpisodes(episode, length);

    max_episodes_[length].insert(std::make_unique<CompositeEpisode>(episode));
}

void MaxEpisodesCollection::SimpleAdd(CompositeEpisode const& episode) {
    size_t length = episode.GetLength();
    while (length >= max_episodes_.size()) {
        max_episodes_.emplace_back();
    }
    max_episodes_[length].insert(std::make_unique<CompositeEpisode>(episode));
}

void MaxEpisodesCollection::BatchAdd(std::vector<MaxEpisodesCollection>& collections) {
    assert(max_episodes_.size() == 0);

    size_t global_max_len = 0;
    for (auto const& col : collections) {
        if (!col.max_episodes_.empty()) {
            global_max_len = std::max(global_max_len, col.max_episodes_.size() - 1);
        }
    }

    while (global_max_len >= max_episodes_.size()) {
        max_episodes_.emplace_back();
    }

    std::vector<std::unique_ptr<CompositeEpisode>> bucket_buffer;

    for (int len = static_cast<int>(global_max_len); len >= 0; --len) {
        size_t length = static_cast<size_t>(len);
        for (auto& collection : collections) {
            if (length >= collection.max_episodes_.size()) {
                continue;
            }

            auto& source_set = collection.max_episodes_[length];
            auto iter = source_set.begin();
            while (iter != source_set.end()) {
                auto node = source_set.extract(iter++);
                if (!node.empty()) {
                    bucket_buffer.push_back(std::move(node.value()));
                }
            }
        }

        if (bucket_buffer.empty()) {
            continue;
        }

        std::sort(bucket_buffer.begin(), bucket_buffer.end(), DescendingCompositeEpisodeComparator{});

        for (auto& ep_ptr : bucket_buffer) {
            if (CheckForSuperEpisode(*ep_ptr, length)) {
                continue;
            }
            max_episodes_[length].insert(std::move(ep_ptr));
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

    for (auto& s : max_episodes_) {
        for (auto const& ptr : s) {
            if (ptr) {
                result.push_back(ptr->GetRaw());
            }
        }
        s.clear();
    }

    max_episodes_.clear();
    return result;
}

bool MaxEpisodesCollection::CheckForSuperEpisode(CompositeEpisode const& episode,
                                                 size_t length) const {
    for (int index = max_episodes_.size() - 1; index >= static_cast<int>(length); --index) {
        auto& max_episodes_set = max_episodes_[index];
        for (auto iter = max_episodes_set.rbegin(); iter != max_episodes_set.rend(); ++iter) {
            auto const& max_episode = *iter;

            if (max_episode->GetEventsSum() < episode.GetEventsSum()) {
                break;
            }

            if (episode.GetEvenEventsSum() <= max_episode->GetEvenEventsSum() &&
                episode.GetOddEventsSum() <= max_episode->GetOddEventsSum() &&
                max_episode->StrictlyContains(episode)) {
                return true;
            }
        }
    }

    return false;
}

void MaxEpisodesCollection::RemoveSubEpisodes(CompositeEpisode const& episode, size_t length) {
    for (size_t index = 1; index <= length; ++index) {
        auto& max_episodes_set = max_episodes_[index];
        auto iter = max_episodes_set.begin();

        while (iter != max_episodes_set.end()) {
            auto const& max_episode = *iter;

            if (max_episode->GetEvenEventsSum() >= episode.GetEventsSum()) {
                break;
            }

            if (episode.GetEvenEventsSum() >= max_episode->GetEvenEventsSum() &&
                episode.GetOddEventsSum() >= max_episode->GetOddEventsSum() &&
                episode.StrictlyContains(*max_episode)) {
                iter = max_episodes_set.erase(iter);
            } else {
                iter++;
            }
        }
    }
}

}  // namespace algos::maxfem
