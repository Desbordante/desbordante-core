#include "composite_episode.h"

namespace algos::tke {

CompositeEpisode::CompositeEpisode(ParallelEpisode const& seed)
    : model::CompositeEpisode({seed.GetEventSetPtr()}),
      bound_list_(std::make_shared<BoundList>(seed)) {}

std::optional<CompositeEpisode> CompositeEpisode::TryExtend(ParallelEpisode const& ext,
                                                            size_t min_support,
                                                            size_t window_length) const {
    std::optional<BoundList> new_bound =
            bound_list_->Extend(ext.GetLocationList(), min_support, window_length);
    if (!new_bound) return std::nullopt;

    CompositeEpisode child;
    child.sequence_ = sequence_;
    child.bound_list_ = std::make_shared<BoundList>(std::move(*new_bound));
    child.sequence_.push_back(ext.GetEventSetPtr());
    return child;
}

CompositeEpisode::RawEpisode CompositeEpisode::GetRaw() const {
    RawEpisode result;
    result.first.reserve(sequence_.size());
    for (auto const& event_set : sequence_) {
        result.first.push_back(event_set->GetEvents());
    }
    result.second = bound_list_->GetSupport();
    return result;
}

}  // namespace algos::tke
