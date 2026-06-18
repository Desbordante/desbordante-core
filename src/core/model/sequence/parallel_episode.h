#pragma once

#include <memory>

#include "core/model/sequence/event.h"
#include "core/model/sequence/event_set.h"

namespace model {

class ParallelEpisode {
protected:
    std::shared_ptr<EventSet> event_set_;

public:
    ParallelEpisode() {
        event_set_ = std::make_shared<EventSet>();
    }

    ParallelEpisode(EventSet event_set)
        : event_set_(std::make_shared<EventSet>(std::move(event_set))) {}

    ParallelEpisode(std::shared_ptr<EventSet> event_set) : event_set_(std::move(event_set)) {}

    Event GetLastEvent() const {
        return event_set_->GetLast();
    }

    EventSet const& GetEventSet() const {
        return *event_set_;
    }

    std::shared_ptr<EventSet> GetEventSetPtr() const {
        return event_set_;
    }
};

}  // namespace model
