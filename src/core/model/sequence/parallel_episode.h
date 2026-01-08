#pragma once

#include "core/model/sequence/event.h"
#include "core/model/sequence/event_set.h"

namespace model {

class ParallelEpisode {
protected:
    EventSet event_set_;

public:
    ParallelEpisode() {}

    ParallelEpisode(EventSet event_set) : event_set_(event_set) {}

    Event GetLastEvent() const {
        return event_set_.GetLast();
    }

    EventSet const& GetEventSet() const {
        return event_set_;
    }
};

}  // namespace model
