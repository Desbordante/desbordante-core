#pragma once

#include "timed_event_set.h"

namespace model {

class ComplexEventSequence {
private:
    std::vector<TimedEventSet> event_sequence_;
public:
    ComplexEventSequence(std::vector<TimedEventSet> event_sequence) : event_sequence_(std::move(event_sequence)) {}

    auto begin() {
        return event_sequence_.begin();
    }

    auto end() {
        return event_sequence_.end();
    }

    auto begin() const {
        return event_sequence_.begin();
    }

    auto end() const {
        return event_sequence_.end();
    }

    size_t Size() const {
        return event_sequence_.size();
    }

    TimedEventSet const& At(size_t index) const {
        return event_sequence_[index];
    }
};

}  // namespace model
