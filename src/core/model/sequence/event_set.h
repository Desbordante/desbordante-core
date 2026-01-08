#pragma once

#include <unordered_map>
#include <vector>

#include "event.h"

namespace model {

class EventSet {
private:
    std::vector<Event> event_set_;

public:
    EventSet() {}

    EventSet(std::vector<Event> event_set) : event_set_(std::move(event_set)) {}

    auto begin() const {
        return event_set_.begin();
    }

    auto end() const {
        return event_set_.end();
    }

    void Add(Event event) {
        event_set_.push_back(event);
    }

    std::vector<Event> const& GetEvents() const {
        return event_set_;
    }

    Event GetLast() const {
        return event_set_.back();
    }

    bool IsSortedUnique() const;

    size_t GetSize() const {
        return event_set_.size();
    }

    void MapEvents(std::vector<Event> const& mapping);

    void MapEventsAndRemoveInfrequent(std::unordered_map<Event, Event> const& mapping);

    bool Includes(EventSet const& other) const;
};

}  // namespace model
