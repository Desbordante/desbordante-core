#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern.h"

#include <algorithm>
#include <numeric>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/hycommon/util/pli_util.h"

namespace algos::cfdfinder {

void Pattern::UpdateCover(boost::dynamic_bitset<> const& used_rows) {
    for (auto& cluster : cover_) {
        std::erase_if(cluster, [&used_rows](int row) { return used_rows.test(row); });
    }

    std::erase_if(cover_, [](auto const& cluster) { return cluster.empty(); });

    support_ = static_cast<double>(GetNumCover());
}

void Pattern::UpdateKeepers(Row const& inverted_pli_rhs) {
    num_keepers_ = support_ - CalculateViolations(inverted_pli_rhs);
}

size_t Pattern::GetNumCover() const {
    return std::accumulate(cover_.begin(), cover_.end(), 0u,
                           [](size_t sum, auto const& cluster) { return sum + cluster.size(); });
}

size_t Pattern::CalculateViolations(Row const& inverted_rhs_pli) const {
    size_t violations = 0;
    std::unordered_map<size_t, size_t> rhs_cluster_counts;
    for (auto const& cluster : cover_) {
        size_t max_cluster_size = 0;

        for (auto tuple : cluster) {
            auto cluster_id = inverted_rhs_pli[tuple];

            if (hy::PLIUtil::IsSingletonCluster(cluster_id)) continue;

            max_cluster_size = std::max(max_cluster_size, ++rhs_cluster_counts[cluster_id]);
        }
        violations +=
                (max_cluster_size > 0) ? (cluster.size() - max_cluster_size) : (cluster.size() - 1);
        rhs_cluster_counts.clear();
    }

    return violations;
}

}  // namespace algos::cfdfinder
