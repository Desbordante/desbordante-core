#include "composite_episode.h"

namespace algos::maxfem {

CompositeEpisode::CompositeEpisode(std::vector<std::shared_ptr<model::EventSet>> sequence)
    : model::CompositeEpisode(std::move(sequence)) {
    for (auto const& event_set : sequence_) {
        CountDataForEventSet(*event_set);
    }
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

void CompositeEpisode::Extend(ParallelEpisode const& parallel_episode) {
    sequence_.push_back(parallel_episode.GetEventSetPtr());
    CountDataForEventSet(*sequence_.back());
}

void CompositeEpisode::Shorten() {
    CountDataForEventSet(*sequence_.back(), false);
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
    result.reserve(sequence_.size());
    for (auto const& event_set : sequence_) {
        result.push_back(event_set->GetEvents());
    }
    return result;
}

bool CompositeEpisodeComparator::operator()(std::unique_ptr<CompositeEpisode> const& lhs,
                                            std::unique_ptr<CompositeEpisode> const& rhs) const {
    if (lhs->GetEventsSum() != rhs->GetEventsSum()) {
        return lhs->GetEventsSum() < rhs->GetEventsSum();
    }
    return lhs < rhs;
}

}  // namespace algos::maxfem
