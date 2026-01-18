#include "bound_list.h"

namespace algos::maxfem {

BoundList::BoundList(ParallelEpisode const& parallel_episode) {
    starts_.reserve(parallel_episode.GetSupport());
    ends_.reserve(parallel_episode.GetSupport());
    for (auto timestamp : parallel_episode.GetLocationList()) {
        starts_.push_back(timestamp);
        ends_.push_back(timestamp);
    }
}

std::optional<BoundList> BoundList::Extend(std::vector<model::Timestamp> const& loc_list,
                                           size_t min_support, size_t window_length) const {
    const size_t max_misses = starts_.size() - min_support;
    size_t current_misses = 0;

    std::vector<model::Timestamp> new_starts;
    std::vector<model::Timestamp> new_ends;
    new_starts.reserve(starts_.size());
    new_ends.reserve(starts_.size());

    for (size_t this_ind = 0, other_ind = 0;
         this_ind < starts_.size() && other_ind < loc_list.size();) {
        auto this_interval_start = starts_[this_ind];
        auto other_location = loc_list[other_ind];

        if (other_location <= ends_[this_ind]) {
            other_ind++;
        } else if (other_location - this_interval_start >= window_length) {
            this_ind++;
            current_misses++;
            if (current_misses > max_misses) {
                return std::nullopt;
            }
        } else {
            new_starts.push_back(this_interval_start);
            new_ends.push_back(other_location);
            this_ind++;
        }
    }

    if (new_starts.size() < min_support) {
        return std::nullopt;
    }

    return BoundList(std::move(new_starts), std::move(new_ends));
}

}  // namespace algos::maxfem
