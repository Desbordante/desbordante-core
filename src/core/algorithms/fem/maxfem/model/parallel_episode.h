#pragma once

#include <vector>
#include <memory>

#include "model/sequence/parallel_episode.h"
#include "algorithms/fem/maxfem/model/location_list.h"
#include "model/sequence/complex_event_sequence.h"

namespace algos::maxfem {

class ParallelEpisode : public model::ParallelEpisode {
private:
    std::shared_ptr<LocationList> location_list_;

public:
    ParallelEpisode() {}

    ParallelEpisode(model::EventSet event_set, std::shared_ptr<LocationList> location_list)
        : model::ParallelEpisode(std::move(event_set)),
        location_list_(std::move(location_list)) {}

    static std::vector<ParallelEpisode> BuildParallelEpisodes(
        model::ComplexEventSequence const& event_sequence,
        std::vector<std::shared_ptr<LocationList>> const& events_location_lists,
        model::Event events_num);

    size_t GetSupport() const;

    ParallelEpisode ParallelExtension(model::Event event, LocationList const& event_location_list) const;
};

}  // namespace algos::maxfem
