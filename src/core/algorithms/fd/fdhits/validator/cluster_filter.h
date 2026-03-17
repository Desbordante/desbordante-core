#pragma once

#include <cstddef>
#include <vector>

#include "core/model/table/position_list_index.h"

namespace algos::fd::fdhits {

using Cluster = model::PositionListIndex::Cluster;

class ClusterFilter {
public:
    virtual ~ClusterFilter() = default;

    virtual bool Keep(Cluster const& cluster) const {
        return cluster.size() > 1;
    }

    virtual void SetRequestedLevel(bool active) = 0;
};

}  // namespace algos::fd::fdhits
