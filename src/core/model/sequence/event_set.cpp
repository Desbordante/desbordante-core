#include "event_set.h"

#include <ranges>

namespace model {

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
