#pragma once

#include <memory>
#include <queue>
#include <vector>

#include "core/algorithms/fem/tke/model/location_list.h"
#include "core/algorithms/fem/tke/model/parallel_episode.h"
#include "core/model/sequence/event.h"

namespace algos::tke {

class ParallelTopKMiner {
public:
    ParallelTopKMiner(model::Event events_num, size_t k,
                      std::vector<std::shared_ptr<LocationList>> events_loc_lists);

    std::vector<ParallelEpisode> Mine();

private:
    struct MaxSupportCmp {
        bool operator()(ParallelEpisode const& a, ParallelEpisode const& b) const {
            return a.GetSupport() < b.GetSupport();
        }
    };

    struct MinSupportCmp {
        bool operator()(ParallelEpisode const& a, ParallelEpisode const& b) const {
            return a.GetSupport() > b.GetSupport();
        }
    };

    using TopK = std::priority_queue<ParallelEpisode, std::vector<ParallelEpisode>, MinSupportCmp>;
    using Explore =
            std::priority_queue<ParallelEpisode, std::vector<ParallelEpisode>, MaxSupportCmp>;

    void TryAdd(ParallelEpisode ep, TopK& top_k, Explore& explore) const;

    model::Event events_num_;
    size_t k_;
    std::vector<std::shared_ptr<LocationList>> events_loc_lists_;
};

}  // namespace algos::tke
