#pragma once

#include <optional>
#include <vector>

#include "core/algorithms/fem/maxfem/model/parallel_episode.h"
#include "core/model/sequence/timestamp.h"

namespace algos::maxfem {

class BoundList {
private:
    using Bound = std::pair<model::Timestamp, model::Timestamp>;

    std::vector<Bound> bound_list_;

    explicit BoundList(std::vector<Bound> bound_list) : bound_list_(std::move(bound_list)) {}

public:
    explicit BoundList(ParallelEpisode const& parallel_episode);

    std::optional<BoundList> Extend(std::vector<model::Timestamp> const& loc_list,
                                    size_t min_support, size_t window_length) const;

    size_t GetSupport() const {
        return bound_list_.size();
    }
};

}  // namespace algos::maxfem
