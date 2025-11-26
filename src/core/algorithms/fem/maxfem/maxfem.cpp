#include "maxfem.h"
#include "util/timed_invoke.h"

namespace algos::maxfem {

void MaxFEM::ResetState() {}

unsigned long long MaxFEM::ExecuteInternal() {
    return util::TimedInvoke(&MaxFEM::FindFrequentEpisodes, this);
}

void MaxFEM::FindFrequentEpisodes() {
    std::vector<size_t> events_supports = GetEventsSupports();
    RemoveInfrequentEpisodes(events_supports);
    std::vector<LocationList> location_lists = BuildLocationLists();
}

std::vector<size_t> MaxFEM::GetEventsSupports() const {
    std::vector<size_t> supports(events_num_, 0);
    for (auto const& event_set : *event_sequence_) {
        for (model::Event const event : event_set) {
            supports[event] += 1;
        }
    }
    return supports;
}

void MaxFEM::RemoveInfrequentEpisodes(std::vector<size_t>& events_supports) {
    std::vector<model::Event> mapping(events_num_, model::kInvalidEvent);
    model::Event new_events_num = model::kStartEvent;

    for (model::Event event = model::kStartEvent; event < events_num_; ++event) {
        if (events_supports[event] >= min_support_) {
            mapping[event] = new_events_num;
            events_supports[new_events_num] = events_supports[event];
            new_events_num++;
        }
    }
    events_supports.resize(new_events_num);
    events_num_ = new_events_num;

    for (auto& event_set : *event_sequence_) {
        event_set.mapEvents(mapping);
    }
}

std::vector<LocationList> MaxFEM::BuildLocationLists() const {
    std::vector<LocationList> location_lists(events_num_);
    for (size_t index = 0; index < event_sequence_->Size(); ++index) {
        for (model::Event const event : event_sequence_->At(index)) {
            location_lists[event].PushBack(index);
        }
    }
    return location_lists;
}

}  // namespace algos::maxfem
