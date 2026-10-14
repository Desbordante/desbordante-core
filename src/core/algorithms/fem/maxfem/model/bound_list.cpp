#include "bound_list.h"

namespace algos::maxfem {

BoundList::BoundList(ParallelEpisode const& parallel_episode) {
    auto const& locs = parallel_episode.GetLocationList();
    bound_list_.reserve(locs.size());
    for (auto ts : locs) {
        bound_list_.push_back({ts, ts});
    }
}

std::optional<BoundList> BoundList::Extend(std::vector<model::Timestamp> const& loc_list,
                                           size_t min_support, size_t window_length) const {
    size_t const max_misses = bound_list_.size() - min_support;
    size_t current_misses = 0;

    std::vector<Bound> new_bounds;
    new_bounds.reserve(bound_list_.size());

    for (size_t this_ind = 0, other_ind = 0;
         this_ind < bound_list_.size() && other_ind < loc_list.size();) {
        auto const [start, end] = bound_list_[this_ind];
        auto const other_loc = loc_list[other_ind];

        if (other_loc <= end) {
            ++other_ind;
        } else if (other_loc - start >= window_length) {
            ++this_ind;
            if (++current_misses > max_misses) return std::nullopt;
        } else {
            new_bounds.push_back({start, other_loc});
            ++this_ind;
        }
    }

    if (new_bounds.size() < min_support) return std::nullopt;

    if (new_bounds.capacity() > 2 * new_bounds.size()) new_bounds.shrink_to_fit();
    return BoundList(std::move(new_bounds));
}

}  // namespace algos::maxfem
