#pragma once

#include <vector>

#include "event_type.h"

namespace model {

class EventSet {
private:
    std::vector<EventType> event_set_;
public:
    EventSet(std::vector<EventType> event_set) : event_set_(std::move(event_set)) {}

    auto begin() const {
        return event_set_.begin();
    }

    auto end() const {
        return event_set_.end();
    }
};

}  // namespace model
