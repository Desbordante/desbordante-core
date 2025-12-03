#pragma once

#include <memory>

#include "algorithms/fem/fem_algorithm.h"
#include "model/sequence/complex_event_sequence.h"
#include "algorithms/fem/maxfem/model/location_list.h"
#include "algorithms/fem/maxfem/model/parallel_episode.h"

namespace algos::maxfem {

class MaxFEM : public FEMAlgorithm {
private:
    std::unique_ptr<model::ComplexEventSequence> event_sequence_;
    size_t window_length_;
    size_t min_support_;
    model::Event events_num_;

    void ResetState() override;

    unsigned long long ExecuteInternal() override;

    void FindFrequentEpisodes();

    std::vector<size_t> GetEventsSupports() const;

    void RemoveInfrequentEvents();

    std::vector<ParallelEpisode> FindFrequentParallelEpisodes() const;

    std::vector<std::shared_ptr<LocationList>> BuildEventsLocationLists() const;

public:
    MaxFEM() {}
};

}
