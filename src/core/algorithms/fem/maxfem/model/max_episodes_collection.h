#pragma once

#include <memory>
#include <set>

#include "core/algorithms/fem/maxfem/model/composite_episode.h"

namespace algos::maxfem {

class MaxEpiosdesCollection {
private:
    std::vector<std::set<std::unique_ptr<CompositeEpisode>, CompositeEpisodeComparator>>
            max_episodes_;

    bool CheckForSuperEpisode(CompositeEpisode const& episode, size_t length) const;

    void RemoveSubEpisodes(CompositeEpisode const& episode, size_t length);

public:
    MaxEpiosdesCollection() {}

    void Add(CompositeEpisode const& episode);

    std::vector<CompositeEpisode::RawEpisode> GetResult(
            std::vector<model::Event> const& mapping,
            std::vector<ParallelEpisode> const& parallel_episodes);
};

}  // namespace algos::maxfem
