#pragma once

#include "core/model/sequence/event_set.h"

namespace model {

class CompositeEpisode {
protected:
    std::vector<EventSet> sequence_;

public:
    CompositeEpisode() {}

    CompositeEpisode(std::vector<EventSet> sequence) : sequence_(std::move(sequence)) {}

    size_t GetLength() const {
        return sequence_.size();
    }

    std::vector<EventSet> const& GetEventSets() const {
        return sequence_;
    }
};

}  // namespace model
