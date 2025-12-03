#include "parallel_episode.h"

namespace algos::maxfem {

std::vector<ParallelEpisode> ParallelEpisode::BuildParallelEpisodes(
        model::ComplexEventSequence const& event_sequence,
        std::vector<std::shared_ptr<LocationList>> const& events_location_lists,
        model::Event events_num) {
    std::vector<ParallelEpisode> parallel_episodes;
    parallel_episodes.reserve(events_num);
    for (model::Event event = model::kStartEvent; event < events_num; ++event) {
        parallel_episodes.emplace_back(
            model::EventSet(std::vector{event}), events_location_lists[event]);
    }
    return parallel_episodes;
}

size_t ParallelEpisode::GetSupport() const {
    return location_list_->Size();
}

ParallelEpisode ParallelEpisode::ParallelExtension(
        model::Event event, LocationList const& event_location_list) const {
    return ParallelEpisode();
}

}  // namespace algos::maxfem
