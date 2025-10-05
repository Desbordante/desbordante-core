#include "violations_util.h"

#include <unordered_map>

#include "algorithms/fd/hycommon/util/pli_util.h"

namespace algos::cfdfinder::util {
size_t CalculateViolations(Cluster const& cluster, hy::Row const& inverted_rhs_pli) {
    size_t max_cluster_size = 0;
    std::unordered_map<size_t, size_t> rhs_cluster_counts;

    for (auto tuple : cluster) {
        auto cluster_id = inverted_rhs_pli[tuple];
        if (hy::PLIUtil::IsSingletonCluster(cluster_id)) continue;

        rhs_cluster_counts[cluster_id] += 1;
        if (max_cluster_size < rhs_cluster_counts[cluster_id])
            max_cluster_size = rhs_cluster_counts[cluster_id];
    }

    return (max_cluster_size > 0) ? (cluster.size() - max_cluster_size) : cluster.size() - 1;
}

}  // namespace algos::cfdfinder::util