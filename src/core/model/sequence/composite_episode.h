#pragma once

#include <memory>

#include "core/model/sequence/event_set.h"

namespace model {

class CompositeEpisode {
protected:
    std::vector<std::shared_ptr<EventSet>> sequence_;

public:
    CompositeEpisode() {}

    CompositeEpisode(std::vector<std::shared_ptr<EventSet>> sequence)
        : sequence_(std::move(sequence)) {}

    size_t GetLength() const {
        return sequence_.size();
    }

    std::vector<std::shared_ptr<EventSet>> const& GetEventSets() const {
        return sequence_;
    }
};

}  // namespace model
