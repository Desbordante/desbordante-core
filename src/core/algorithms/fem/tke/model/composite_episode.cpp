#include "composite_episode.h"

namespace algos::tke {

CompositeEpisode::CompositeEpisode(ParallelEpisode const& seed)
    : model::CompositeEpisode({seed.GetEventSetPtr()}),
      bound_list_(std::make_shared<BoundList>(seed)) {
    CountDataForEventSet(*seed.GetEventSetPtr());
}

std::optional<CompositeEpisode> CompositeEpisode::TryExtend(ParallelEpisode const& ext,
                                                             size_t min_support,
                                                             size_t window_length) const {
    std::optional<BoundList> new_bound =
            bound_list_->Extend(ext.GetLocationList(), min_support, window_length);
    if (!new_bound) return std::nullopt;

    CompositeEpisode child;
    child.sequence_ = sequence_;
    child.sum_of_even_items_ = sum_of_even_items_;
    child.sum_of_odd_items_ = sum_of_odd_items_;
    child.events_count_ = events_count_;
    child.bound_list_ = std::make_shared<BoundList>(std::move(*new_bound));
    child.sequence_.push_back(ext.GetEventSetPtr());
    child.CountDataForEventSet(*child.sequence_.back());
    return child;
}

void CompositeEpisode::CountDataForEventSet(model::EventSet const& event_set, bool add) {
    int sign = add ? 1 : -1;
    for (model::Event event : event_set) {
        if (event % 2 == 0) {
            sum_of_even_items_ += sign * (event + 1);
        } else {
            sum_of_odd_items_ += sign * (event + 1);
        }
    }
    events_count_ += sign * event_set.GetSize();
}

bool CompositeEpisode::StrictlyContains(CompositeEpisode const& other) const {
    if (events_count_ <= other.events_count_) {
        return false;
    }

    size_t this_pos = 0;
    size_t other_pos = 0;
    while (this_pos < GetLength() && other_pos < other.GetLength()) {
        if (sequence_[this_pos]->Includes(*other.sequence_[other_pos])) {
            other_pos++;
            if (other_pos == other.GetLength()) {
                return true;
            }
        }

        this_pos++;
        if (this_pos >= GetLength()) {
            return false;
        }

        size_t remaining_in_this = GetLength() - this_pos;
        size_t remaining_in_other = other.GetLength() - other_pos;

        if (remaining_in_this < remaining_in_other) {
            return false;
        }
    }

    return other_pos == other.GetLength();
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
