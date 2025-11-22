#include "algorithms/cfd/cfdfinder/model/pruning/partial_fd_pruning.h"

#include <cmath>
#include <unordered_map>

#include "algorithms/fd/hycommon/util/pli_util.h"

namespace algos::cfdfinder {
double PartialFdPruning::CalculateG1(Pattern const& pattern) const {
    unsigned long long violations = CalculateViolations(pattern);
    double normalization = std::pow(num_records_, 2) - num_records_;
    return violations / normalization;
}

unsigned long long PartialFdPruning::CalculateViolations(Pattern const& pattern) const {
    unsigned long long violations = 0;

    for (auto const& cluster : pattern.GetCover()) {
        size_t cluster_size = cluster.size();
        std::unordered_map<int, size_t> value_counts;

        for (auto index : cluster) {
            auto value = inverted_pli_rhs_[index];
            if (!algos::hy::PLIUtil::IsSingletonCluster(value)) {
                ++value_counts[value];
            }
        }

        size_t total_pairs = cluster_size * cluster_size;

        size_t non_violation_pairs = 0;
        for (auto const& entry : value_counts) {
            non_violation_pairs += entry.second * entry.second;
        }

        size_t cluster_violations = total_pairs - non_violation_pairs;
        violations += cluster_violations;
    }

    return violations;
}
}  // namespace algos::cfdfinder
