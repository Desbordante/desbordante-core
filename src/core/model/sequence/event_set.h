#pragma once

#include <vector>

#include "event.h"

namespace model {

class EventSet {
private:
    std::vector<Event> event_set_;
public:
    EventSet(std::vector<Event> event_set) : event_set_(std::move(event_set)) {}

    auto begin() const {
        return event_set_.begin();
    }

    auto end() const {
        return event_set_.end();
    }

    void mapEvents(std::vector<Event> const& mapping);
};

}  // namespace model
