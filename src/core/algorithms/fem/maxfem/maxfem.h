#pragma once

#include <map>
#include <memory>
#include <unordered_map>

#include "core/algorithms/fem/fem_algorithm.h"
#include "core/algorithms/fem/maxfem/model/bound_list.h"
#include "core/algorithms/fem/maxfem/model/location_list.h"
#include "core/algorithms/fem/maxfem/model/max_episodes_collection.h"
#include "core/algorithms/fem/maxfem/model/parallel_episode.h"

namespace algos::maxfem {

class MaxFEM : public FEMAlgorithm {
private:
    size_t window_length_;
    size_t min_support_;
    model::Event events_num_ = 0;
    std::vector<CompositeEpisode::RawEpisode> max_frequent_episodes_;
    std::unordered_map<model::Event, model::Event> mapping_;
    std::vector<model::Event> reverse_mapping_;

    MaxEpisodesCollection max_episodes_collection_;

    void ResetState() override;

    unsigned long long ExecuteInternal() override;

    void FindFrequentEpisodes();

    std::map<model::Event, size_t> GetEventsSupports() const;

    void RemoveInfrequentEvents();

    std::vector<ParallelEpisode> FindFrequentParallelEpisodes() const;

    std::vector<std::shared_ptr<LocationList>> BuildEventsLocationLists() const;

    void FindFrequentParallelEpisodesRecursive(
            ParallelEpisode const& current_episode,
            std::vector<std::shared_ptr<LocationList>> const& events_loc_lists,
            std::vector<ParallelEpisode>& results) const;

    void FindFrequentCompositeEpisodes(std::vector<ParallelEpisode> const& parallel_episodes);

protected:
    void MakeExecuteOptsAvailable() override;

public:
    MaxFEM();

    std::vector<CompositeEpisode::RawEpisode> const& GetMaxFrequentEpisodes() const {
        return max_frequent_episodes_;
    }
};

}  // namespace algos::maxfem
