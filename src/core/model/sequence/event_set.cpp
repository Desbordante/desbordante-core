#include "event_set.h"

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
    auto write_iter = event_set_.begin();
    for (auto const& event : event_set_) {
        Event new_event = mapping[event];
        if (new_event != kInvalidEvent) {
            *write_iter = new_event;
            ++write_iter;
        }
    }
    event_set_.erase(write_iter, event_set_.end());
}

}  // namespace model
