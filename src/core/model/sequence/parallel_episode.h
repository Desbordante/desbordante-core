#pragma once

#include "event_set.h"

namespace model {

class ParallelEpisode {
protected:
    EventSet event_set_;

public:
    ParallelEpisode() {}

    ParallelEpisode(EventSet event_set) : event_set_(event_set) {}

    GetLastEvent() const {
        return event_set_.GetLast();
    }
};

}  // namespace model
