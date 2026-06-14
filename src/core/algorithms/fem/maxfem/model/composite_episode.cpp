#include "composite_episode.h"

namespace algos::maxfem {

CompositeEpisode::CompositeEpisode(std::vector<std::shared_ptr<model::EventSet>> sequence,
                                   size_t support)
    : model::CompositeEpisode(std::move(sequence)), support_(support) {
    for (auto const& event_set : sequence_) {
        CountDataForEventSet(*event_set);
    }
}

void CompositeEpisode::CountDataForEventSet(model::EventSet const& event_set, bool add) {
    for (model::Event event : event_set) {
        model::Event val = event + 1;
        if (event % 2 == 0) {
            if (add) {
                sum_of_even_items_ += val;
            } else {
                sum_of_even_items_ -= val;
            }
        } else {
            if (add) {
                sum_of_odd_items_ += val;
            } else {
                sum_of_odd_items_ -= val;
            }
        }
    }
    if (add) {
        events_count_ += event_set.GetSize();
    } else {
        events_count_ -= event_set.GetSize();
    }
}

void CompositeEpisode::Extend(ParallelEpisode const& parallel_episode, size_t new_support) {
    sequence_.push_back(parallel_episode.GetEventSetPtr());
    support_ = new_support;
    CountDataForEventSet(*sequence_.back());
}

void CompositeEpisode::Shorten(size_t new_support) {
    CountDataForEventSet(*sequence_.back(), false);
    support_ = new_support;
    sequence_.pop_back();
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
    result.second = support_;
    return result;
}

}  // namespace algos::maxfem
