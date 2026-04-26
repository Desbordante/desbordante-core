#pragma once

#include <map>
#include <vector>

#include "core/algorithms/fem/fem_algorithm.h"
#include "core/algorithms/fem/tke/model/composite_episode.h"
#include "core/algorithms/fem/tke/model/location_list.h"
#include "core/config/thread_number/type.h"

namespace algos::tke {

class TKE : public FEMAlgorithm {
private:
    size_t window_length_;
    size_t episodes_num_;
    config::ThreadNumType threads_num_;
    model::Event events_num_ = 0;
    std::vector<CompositeEpisode::RawEpisode> top_k_frequent_episodes_;
    std::vector<model::Event> reverse_mapping_;

    void ResetState() override;

    unsigned long long ExecuteInternal() override;

    void FindFrequentEpisodes();

    std::map<model::Event, size_t> GetEventsSupports() const;

    void RemoveInfrequentEvents(std::map<model::Event, size_t> const& events_supports,
                                        size_t event_minsup);

    std::vector<std::shared_ptr<LocationList>> BuildEventsLocationLists() const;

    void DecodeAndStoreResults(std::vector<CompositeEpisode>&& composites);

protected:
    void MakeExecuteOptsAvailable() override;

public:
    TKE();

    std::vector<CompositeEpisode::RawEpisode> const& GetTopKFrequentEpisodes() const {
        return top_k_frequent_episodes_;
    }
};

}  // namespace algos::tke
