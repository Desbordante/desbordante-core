#pragma once

#include "event_set.h"

namespace model {

class CompositeEpisode {
private:
    std::vector<EventSet> sequence_;

public:
    CompositeEpisode(std::vector<EventSet> sequence) : sequence_(std::move(sequence)) {}
};

}  // namespace model
