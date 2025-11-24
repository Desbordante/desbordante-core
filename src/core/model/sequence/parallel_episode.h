#pragma once

#include "event_set.h"

namespace model {

class ParallelEpisode {
private:
    EventSet event_set_;
public:
    ParallelEpisode(EventSet event_set) : event_set_(event_set) {}
};

}  // namespace model
