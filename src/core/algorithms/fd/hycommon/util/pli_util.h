#pragma once

#include <cstddef>

#include "core/algorithms/fd/hycommon/types.h"

namespace algos::hy {

class PLIUtil {
public:
    static constexpr ClusterId kSingletonClusterId = -1;

    static bool IsSingletonCluster(size_t cluster_id) {
        return cluster_id == kSingletonClusterId;
    }
};

}  // namespace algos::hy
