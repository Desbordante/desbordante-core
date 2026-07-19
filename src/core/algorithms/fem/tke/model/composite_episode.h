#pragma once

#include <memory>
#include <optional>

#include "core/algorithms/fem/tke/model/bound_list.h"
#include "core/algorithms/fem/tke/model/parallel_episode.h"
#include "core/model/sequence/composite_episode.h"

namespace algos::tke {

class CompositeEpisode : public model::CompositeEpisode {
private:
    model::Event sum_of_even_items_ = 0;
    model::Event sum_of_odd_items_ = 0;
    size_t events_count_ = 0;
    std::shared_ptr<BoundList> bound_list_;

    void CountDataForEventSet(model::EventSet const& event_set, bool add = true);

public:
    CompositeEpisode() {}

    explicit CompositeEpisode(ParallelEpisode const& seed);

    std::optional<CompositeEpisode> TryExtend(ParallelEpisode const& ext, size_t min_support,
                                              size_t window_length) const;

    size_t GetSupport() const {
        return bound_list_->GetSupport();
    }

    model::Event GetEventsSum() const {
        return sum_of_even_items_ + sum_of_odd_items_;
    }

    model::Event GetEvenEventsSum() const {
        return sum_of_even_items_;
    }

    model::Event GetOddEventsSum() const {
        return sum_of_odd_items_;
    }

    bool StrictlyContains(CompositeEpisode const& other) const;

    using RawEpisode = std::pair<std::vector<std::vector<model::Event>>, size_t>;

    RawEpisode GetRaw() const;
};

}  // namespace algos::tke
