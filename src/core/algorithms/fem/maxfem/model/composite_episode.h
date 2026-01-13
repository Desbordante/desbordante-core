#pragma once

#include "core/algorithms/fem/maxfem/model/parallel_episode.h"
#include "core/model/sequence/composite_episode.h"

namespace algos::maxfem {

class CompositeEpisode : public model::CompositeEpisode {
private:
    model::Event sum_of_even_items_ = 0;
    model::Event sum_of_odd_items_ = 0;
    size_t events_count_ = 0;

    void CountDataForEventSet(model::EventSet const& event_set, bool add = true);

public:
    CompositeEpisode() {}

    CompositeEpisode(std::vector<std::shared_ptr<model::EventSet>> sequence);

    void Extend(ParallelEpisode const& parallel_episode);

    void Shorten();

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

    using RawEpisode = std::vector<std::vector<model::Event>>;

    RawEpisode GetRaw() const;
};

struct CompositeEpisodeComparator {
    bool operator()(std::unique_ptr<CompositeEpisode> const& lhs,
                    std::unique_ptr<CompositeEpisode> const& rhs) const;
};

}  // namespace algos::maxfem
