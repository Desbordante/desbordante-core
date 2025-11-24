#pragma once

#include "event_set.h"
#include "timestamp.h"

namespace model {

class TimedEventSet : public EventSet {
private:
    Timestamp timestamp_;
public:
    TimedEventSet(std::vector<EventType> event_set, Timestamp timestamp)
        : EventSet(std::move(event_set)), timestamp_(timestamp) {
    }

    Timestamp GetTimestamp() {
        return timestamp_;
    }
};

}  // namespace model
