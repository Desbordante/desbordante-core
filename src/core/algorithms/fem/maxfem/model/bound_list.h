#pragma once

#include <optional>
#include <vector>

#include "core/algorithms/fem/maxfem/model/parallel_episode.h"
#include "core/model/sequence/timestamp.h"

namespace algos::maxfem {

class BoundList {
private:
    std::vector<model::Timestamp> starts_;
    std::vector<model::Timestamp> ends_;

public:
    BoundList(std::vector<model::Timestamp> starts, std::vector<model::Timestamp> ends)
        : starts_(std::move(starts)), ends_(std::move(ends)) {}

    BoundList(ParallelEpisode const& parallel_episode);

    std::optional<BoundList> Merge(BoundList const& other, size_t min_support,
                                   size_t window_length) const;

    size_t GetSupport() const {
        return starts_.size();
    }
};

}  // namespace algos::maxfem
