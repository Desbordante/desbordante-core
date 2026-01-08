#include "composite_episode.h"

namespace algos::maxfem {

CompositeEpisode::CompositeEpisode(std::vector<model::EventSet> sequence)
    : model::CompositeEpisode(std::move(sequence)) {
    for (auto const& event_set : sequence_) {
        CountDataForEventSet(event_set);
    }
}

void CompositeEpisode::CountDataForEventSet(model::EventSet const& event_set, bool add) {
    int sign = add ? 1 : -1;
    for (model::Event event : event_set) {
        if (event % 2 == 0) {
            sum_of_even_items_ += sign * event;
        } else {
            sum_of_odd_items_ += sign * event;
        }
    }
    events_count_ += sign * event_set.GetSize();
}

void CompositeEpisode::Extend(ParallelEpisode const& parallel_episode) {
    sequence_.push_back(parallel_episode.GetEventSet());
    CountDataForEventSet(sequence_.back());
}

void CompositeEpisode::Shorten() {
    CountDataForEventSet(sequence_.back(), false);
    sequence_.pop_back();
}

bool CompositeEpisode::StrictlyContains(CompositeEpisode const& other) const {
    if (events_count_ <= other.events_count_) {
        return false;
    }

    size_t this_pos = 0;
    size_t other_pos = 0;
    while (this_pos < GetLength() && other_pos < other.GetLength()) {
        if (sequence_[this_pos].Includes(other.sequence_[other_pos])) {
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

void CompositeEpisode::MapEvents(std::vector<model::Event> const& mapping) {
    for (auto& event_set : sequence_) {
        event_set.MapEvents(mapping);
    }
}

bool CompositeEpisodeComparator::operator()(std::shared_ptr<CompositeEpisode> const& lhs,
                                            std::shared_ptr<CompositeEpisode> const& rhs) const {
    if (lhs->GetEventsSum() != rhs->GetEventsSum()) {
        return lhs->GetEventsSum() < rhs->GetEventsSum();
    }
    return lhs < rhs;
}

}  // namespace algos::maxfem
