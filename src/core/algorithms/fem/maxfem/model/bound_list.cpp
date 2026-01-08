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
    std::vector<std::pair<model::Timestamp, model::Timestamp>> new_bound_list;
    size_t remaining = bound_list_.size();

    for (size_t this_ind = 0, other_ind = 0;
         this_ind < bound_list_.size() && other_ind < other.bound_list_.size();) {
        if (new_bound_list.size() + remaining < min_support) {
            return std::nullopt;
        }

        auto const& [this_interval_start, this_interval_end] = bound_list_[this_ind];
        auto const& [other_interval_start, other_interval_end] = other.bound_list_[other_ind];

        if (other_interval_end <= this_interval_end) {
            other_ind++;
        } else if (other_interval_end - this_interval_end >= window_length) {
            this_ind++;
            remaining--;
        } else {
            new_bound_list.emplace_back(this_interval_start, other_interval_end);
            this_ind++;
            remaining--;
        }
    }

    return BoundList(std::move(new_bound_list));
}

}  // namespace algos::maxfem
