#pragma once

#include <queue>
#include <vector>

#include "core/algorithms/fem/tke/model/composite_episode.h"
#include "core/algorithms/fem/tke/model/parallel_episode.h"

namespace algos::tke {

class CompositeTopKMiner {
public:
    CompositeTopKMiner(size_t k, size_t window_length, size_t threads_num = 1);

    std::vector<CompositeEpisode> Mine(std::vector<ParallelEpisode>&& parallel_episodes,
                                       size_t initial_minsup = 1);

private:
    struct MaxSupportCmp {
        bool operator()(CompositeEpisode const& a, CompositeEpisode const& b) const {
            return a.GetSupport() < b.GetSupport();
        }
    };

    struct MinSupportCmp {
        bool operator()(CompositeEpisode const& a, CompositeEpisode const& b) const {
            return a.GetSupport() > b.GetSupport();
        }
    };

    using TopK =
            std::priority_queue<CompositeEpisode, std::vector<CompositeEpisode>, MinSupportCmp>;
    using Explore =
            std::priority_queue<CompositeEpisode, std::vector<CompositeEpisode>, MaxSupportCmp>;

    bool TryAdd(CompositeEpisode ep, TopK& top_k, Explore& explore, size_t floor_minsup) const;

    void ExploreSequential(std::vector<ParallelEpisode> const& parallel_episodes, TopK& top_k,
                           Explore& explore, size_t initial_minsup) const;

    void ExploreParallel(std::vector<ParallelEpisode> const& parallel_episodes, TopK& top_k,
                         Explore& explore, size_t initial_minsup) const;

    size_t k_;
    size_t window_length_;
    size_t threads_num_;
};

}  // namespace algos::tke
