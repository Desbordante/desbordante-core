#pragma once

namespace algos::hyfd {

class PLIUtil {
public:
    static constexpr size_t kSingletonClusterId = -1;

    static inline bool IsSingletonCluster(size_t cluster_id) {
        return cluster_id == kSingletonClusterId;
    }
};

}  // namespace algos::hyfd