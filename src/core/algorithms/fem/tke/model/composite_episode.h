#pragma once

#include <memory>
#include <optional>

#include "core/algorithms/fem/tke/model/bound_list.h"
#include "core/algorithms/fem/tke/model/parallel_episode.h"
#include "core/model/sequence/composite_episode.h"

namespace algos::tke {

class CompositeEpisode : public model::CompositeEpisode {
private:
    std::shared_ptr<BoundList> bound_list_;

public:
    CompositeEpisode() {}

    explicit CompositeEpisode(ParallelEpisode const& seed);

    std::optional<CompositeEpisode> TryExtend(ParallelEpisode const& ext, size_t min_support,
                                              size_t window_length) const;

    size_t GetSupport() const {
        return bound_list_->GetSupport();
    }

    using RawEpisode = std::pair<std::vector<std::vector<model::Event>>, size_t>;

    RawEpisode GetRaw() const;
};

}  // namespace algos::tke
