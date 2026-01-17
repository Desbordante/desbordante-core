#include "bound_list.h"

namespace algos::maxfem {

BoundList::BoundList(ParallelEpisode const& parallel_episode) {
    bound_list_.reserve(parallel_episode.GetSupport());
    for (auto timestamp : parallel_episode.GetLocationList()) {
        bound_list_.emplace_back(timestamp, timestamp);
    }
}

std::optional<BoundList> BoundList::Merge(BoundList const& other, size_t min_support,
                                          size_t window_length) const {
    const size_t max_misses = bound_list_.size() - min_support;
    size_t current_misses = 0;

    std::vector<std::pair<model::Timestamp, model::Timestamp>> new_bound_list;
    new_bound_list.reserve(bound_list_.size());

    for (size_t this_ind = 0, other_ind = 0;
         this_ind < bound_list_.size() && other_ind < other.bound_list_.size();) {
        auto const& [this_interval_start, this_interval_end] = bound_list_[this_ind];
        auto const& [other_interval_start, other_interval_end] = other.bound_list_[other_ind];

        if (other_interval_end <= this_interval_end) {
            other_ind++;
        } else if (other_interval_end - this_interval_start >= window_length) {
            this_ind++;
            current_misses++;
            if (current_misses > max_misses) {
                return std::nullopt;
            }
        } else {
            new_bound_list.emplace_back(this_interval_start, other_interval_end);
            this_ind++;
        }
    }

    if (new_bound_list.size() < min_support) {
        return std::nullopt;
    }

    return BoundList(std::move(new_bound_list));
}

}  // namespace algos::maxfem
