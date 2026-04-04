#pragma once

#include <map>
#include <memory>
#include <unordered_map>

#include "core/algorithms/fem/fem_algorithm.h"
#include "core/algorithms/fem/tke/model/location_list.h"
#include "core/algorithms/fem/tke/model/composite_episode.h"
#include "core/algorithms/fem/tke/model/parallel_episode.h"
#include "core/config/thread_number/type.h"

namespace algos::tke {

class TKE : public FEMAlgorithm {
private:
    size_t window_length_;
    size_t episodes_num_;
    config::ThreadNumType threads_num_;

    size_t current_min_support_ = 0;
    model::Event events_num_ = 0;
    std::vector<CompositeEpisode::RawEpisode> top_k_frequent_episodes_;
    std::unordered_map<model::Event, model::Event> mapping_;
    std::vector<model::Event> reverse_mapping_;

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
    TKE();

    std::vector<CompositeEpisode::RawEpisode> const& GetTopKFrequentEpisodes() const {
        return top_k_frequent_episodes_;
    }
};

}  // namespace algos::tke
