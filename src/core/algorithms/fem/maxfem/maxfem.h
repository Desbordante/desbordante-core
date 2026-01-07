#pragma once

#include <memory>

#include "core/algorithms/fem/fem_algorithm.h"
#include "core/algorithms/fem/maxfem/model/location_list.h"
#include "core/algorithms/fem/maxfem/model/parallel_episode.h"

namespace algos::maxfem {

class MaxFEM : public FEMAlgorithm {
private:
    size_t window_length_;
    size_t min_support_;
    model::Event events_num_ = 0;
    std::vector<ParallelEpisode> frequent_episodes_;

    void ResetState() override;

    unsigned long long ExecuteInternal() override;

    void FindFrequentEpisodes();

    std::vector<size_t> GetEventsSupports() const;

    void RemoveInfrequentEvents();

    std::vector<ParallelEpisode> FindFrequentParallelEpisodes() const;

    std::vector<std::shared_ptr<LocationList>> BuildEventsLocationLists() const;

    void FindFrequentEpisodesRecursive(
            ParallelEpisode const& current_episode,
            std::vector<std::shared_ptr<LocationList>> const& events_loc_lists,
            std::vector<ParallelEpisode>& results) const;

protected:
    void MakeExecuteOptsAvailable() override;

public:
    MaxFEM();

    std::vector<ParallelEpisode> const& GetFrequentEpisodes() const {
        return frequent_episodes_;
    }
};

}  // namespace algos::maxfem
