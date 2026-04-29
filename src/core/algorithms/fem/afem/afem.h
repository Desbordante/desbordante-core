#pragma once

#include <map>
#include <memory>
#include <vector>

#include "core/algorithms/fem/fem_algorithm.h"
#include "core/algorithms/fem/maxfem/model/composite_episode.h"
#include "core/algorithms/fem/maxfem/model/location_list.h"
#include "core/algorithms/fem/maxfem/model/parallel_episode.h"
#include "core/config/thread_number/type.h"

namespace algos::afem {

class AFEM : public FEMAlgorithm {
private:
    size_t window_length_;
    size_t min_support_;
    config::ThreadNumType threads_num_;
    double tasks_num_multiplier_;

    model::Event events_num_ = 0;
    std::vector<maxfem::CompositeEpisode::RawEpisode> frequent_episodes_;
    std::vector<model::Event> reverse_mapping_;

    void ResetState() override;

    unsigned long long ExecuteInternal() override;

    void FindFrequentEpisodes();

    std::map<model::Event, size_t> GetEventsSupports() const;

    void RemoveInfrequentEvents();

    std::vector<maxfem::ParallelEpisode> FindFrequentParallelEpisodes() const;

    std::vector<std::shared_ptr<maxfem::LocationList>> BuildEventsLocationLists() const;

    void FindFrequentParallelEpisodesRecursive(
            maxfem::ParallelEpisode const& current_episode,
            std::vector<std::shared_ptr<maxfem::LocationList>> const& events_loc_lists,
            std::vector<maxfem::ParallelEpisode>& results) const;

    void FindFrequentCompositeEpisodes(
            std::vector<maxfem::ParallelEpisode> const& parallel_episodes);

protected:
    void MakeExecuteOptsAvailable() override;

public:
    AFEM();

    std::vector<maxfem::CompositeEpisode::RawEpisode> const& GetFrequentEpisodes() const {
        return frequent_episodes_;
    }
};

}  // namespace algos::afem
