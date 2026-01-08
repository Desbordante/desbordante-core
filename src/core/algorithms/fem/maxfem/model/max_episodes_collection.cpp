#include "max_episodes_collection.h"

namespace algos::maxfem {

void MaxEpiosdesCollection::Add(CompositeEpisode const& episode) {
    size_t length = episode.GetLength();

    if (CheckForSuperEpisode(episode, length)) {
        return;
    }
    while (length >= max_episodes_.size()) {
        max_episodes_.emplace_back();
    }
    RemoveSubEpisodes(episode, length);

    max_episodes_[length].insert(std::make_shared<CompositeEpisode>(episode));
}

std::vector<model::CompositeEpisode> MaxEpiosdesCollection::GetResult(
        std::vector<model::Event> const& mapping) {
    std::vector<model::CompositeEpisode> result;

    size_t total_count = 0;
    for (auto const& s : max_episodes_) {
        total_count += s.size();
    }
    result.reserve(total_count);

    for (auto& s : max_episodes_) {
        for (auto const& ptr : s) {
            if (ptr) {
                ptr->MapEvents(mapping);
                result.push_back(std::move(*ptr));
            }
        }
        s.clear();
    }

    max_episodes_.clear();
    return result;
}

bool MaxEpiosdesCollection::CheckForSuperEpisode(CompositeEpisode const& episode,
                                                 size_t length) const {
    for (int index = max_episodes_.size() - 1; index > static_cast<int>(length); --index) {
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

void MaxEpiosdesCollection::RemoveSubEpisodes(CompositeEpisode const& episode, size_t length) {
    for (size_t index = 1; index < length; ++index) {
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
