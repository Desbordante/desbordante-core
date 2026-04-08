#pragma once

#include <optional>
#include <vector>

#include "core/algorithms/fem/tke/model/parallel_episode.h"
#include "core/model/sequence/timestamp.h"

namespace algos::tke {

class BoundList {
private:
    std::vector<model::Timestamp> starts_;
    std::vector<model::Timestamp> ends_;

public:
    BoundList() = default;

    BoundList(std::vector<model::Timestamp> starts, std::vector<model::Timestamp> ends)
        : starts_(std::move(starts)), ends_(std::move(ends)) {}

    BoundList(ParallelEpisode const& parallel_episode);

    std::optional<BoundList> Extend(std::vector<model::Timestamp> const& loc_list,
                                    size_t min_support, size_t window_length) const;

    size_t GetSupport() const {
        return starts_.size();
    }
};

}  // namespace algos::tke
