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

    for (int len = static_cast<int>(global_max_len); len >= 0; --len) {
        for (auto& collection : collections) {
            if (len >= collection.max_episodes_.size()) {
                continue;
            }

            auto& source_set = collection.max_episodes_[len];
            auto it = source_set.begin();
            while (it != source_set.end()) {
                auto node = source_set.extract(it++);
                if (node.empty()) {
                    continue;
                }

                std::unique_ptr<CompositeEpisode>& ep_ptr = node.value();
                if (CheckForSuperEpisode(*ep_ptr, len)) {
                    continue;
                }
                max_episodes_[len].insert(std::move(node));
            }
        }
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
