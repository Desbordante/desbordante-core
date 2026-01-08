#include "parallel_episode.h"

namespace algos::maxfem {

std::vector<ParallelEpisode> ParallelEpisode::BuildParallelEpisodesWithEvents(
        std::vector<std::shared_ptr<LocationList>> const& events_location_lists,
        model::Event events_num) {
    std::vector<ParallelEpisode> parallel_episodes;
    parallel_episodes.reserve(events_num);
    for (model::Event event = model::kStartEvent; event < events_num; ++event) {
        parallel_episodes.emplace_back(model::EventSet(std::vector{event}),
                                       events_location_lists[event]);
    }
    return parallel_episodes;
}

size_t ParallelEpisode::GetSupport() const {
    return location_list_->Size();
}

ParallelEpisode ParallelEpisode::ParallelExtension(model::Event event,
                                                   LocationList const& event_location_list) const {
    model::EventSet new_event_set = event_set_;
    new_event_set.Add(event);
    auto new_loc_list = location_list_->Merge(event_location_list);
    return ParallelEpisode(std::move(new_event_set), std::move(new_loc_list));
}

}  // namespace algos::maxfem
