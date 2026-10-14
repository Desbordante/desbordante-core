#pragma once

#include <set>

#include "core/algorithms/fem/maxfem/model/composite_episode.h"

namespace algos::maxfem {

class MaxEpisodesCollection {
private:
    using EpisodeSet = std::multiset<CompositeEpisode, CompositeEpisodeComparator>;
    std::vector<EpisodeSet> max_episodes_;

    bool CheckForSuperEpisode(CompositeEpisode const& episode, size_t length) const;

public:
    MaxEpisodesCollection() {}

    void SimpleAdd(CompositeEpisode const& episode);

    void BatchFill(std::vector<MaxEpisodesCollection>& other);

    std::vector<CompositeEpisode::RawEpisode> GetResult(
            std::vector<model::Event> const& mapping,
            std::vector<ParallelEpisode> const& parallel_episodes);
};

}  // namespace algos::maxfem
