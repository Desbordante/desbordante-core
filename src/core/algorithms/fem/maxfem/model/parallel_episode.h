#pragma once

#include "core/algorithms/fem/maxfem/model/location_list.h"
#include "core/model/sequence/complex_event_sequence.h"
#include "core/model/sequence/parallel_episode.h"

namespace algos::maxfem {

class ParallelEpisode : public model::ParallelEpisode {
private:
    std::shared_ptr<LocationList> location_list_;

public:
    ParallelEpisode() {}

    ParallelEpisode(model::EventSet event_set, std::shared_ptr<LocationList> location_list)
        : model::ParallelEpisode(std::move(event_set)), location_list_(std::move(location_list)) {}

    ParallelEpisode(std::shared_ptr<model::EventSet> event_set,
                    std::shared_ptr<LocationList> location_list)
        : model::ParallelEpisode(std::move(event_set)), location_list_(std::move(location_list)) {}

    static std::vector<ParallelEpisode> BuildParallelEpisodesWithEvents(
            std::vector<std::shared_ptr<LocationList>> const& events_location_lists,
            model::Event events_num);

    size_t GetSupport() const;

    ParallelEpisode ParallelExtension(model::Event event,
                                      LocationList const& event_location_list) const;

    std::vector<model::Timestamp> GetLocationList() const {
        return location_list_->GetLocationList();
    }
};

}  // namespace algos::maxfem
