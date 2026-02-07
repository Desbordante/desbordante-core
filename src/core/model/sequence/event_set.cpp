#include "event_set.h"

#include <algorithm>

namespace model {

bool EventSet::IsSortedUnique() const {
    if (event_set_.empty()) return true;
    for (size_t i = 1; i < event_set_.size(); ++i) {
        if (event_set_[i] <= event_set_[i - 1]) {
            return false;
        }
    }
    return true;
}

void EventSet::MapEvents(std::vector<Event> const& mapping) {
    for (auto& event : event_set_) {
        event = mapping[event];
    }
}

void EventSet::MapEventsAndRemoveInfrequent(std::unordered_map<Event, Event> const& mapping) {
    auto write_iter = event_set_.begin();
    for (auto const& event : event_set_) {
        auto new_event = mapping.find(event);
        if (new_event != mapping.end()) {
            *write_iter = new_event->second;
            ++write_iter;
        }
    }
    event_set_.erase(write_iter, event_set_.end());
}

bool EventSet::Includes(EventSet const& other) const {
    return std::includes(event_set_.begin(), event_set_.end(), other.event_set_.begin(),
                         other.event_set_.end());
}

}  // namespace model
