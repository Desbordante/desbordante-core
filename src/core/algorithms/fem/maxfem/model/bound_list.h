#pragma once

#include <optional>
#include <vector>

#include "core/algorithms/fem/maxfem/model/parallel_episode.h"
#include "core/model/sequence/timestamp.h"

namespace algos::maxfem {

class BoundList {
private:
    std::vector<std::pair<model::Timestamp, model::Timestamp>> bound_list_;

public:
    BoundList(std::vector<std::pair<model::Timestamp, model::Timestamp>> bound_list)
        : bound_list_(std::move(bound_list)) {}

    BoundList(ParallelEpisode const& parallel_episode);

    std::optional<BoundList> Merge(BoundList const& other, size_t min_support,
                                   size_t window_length) const;

    size_t GetSupport() const {
        return bound_list_.size();
    }
};

}  // namespace algos::maxfem
