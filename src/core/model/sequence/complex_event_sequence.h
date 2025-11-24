#pragma once

#include "timed_event_set.h"

namespace model {

class ComplexEventSequence {
private:
    std::vector<TimedEventSet> event_sequence_;
public:
    ComplexEventSequence(std::vector<TimedEventSet> event_sequence) : event_sequence_(std::move(event_sequence)) {}
};

}  // namespace model
