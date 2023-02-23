#pragma once

#include <cstddef>

#include "util/position_list_index.h"

namespace algos::fd_verifier {

/* FDVerifier Highlight represents a cluster that violate the FD and provides the information about
 * that cluster */
class Highlight {
private:
    util::PLI::Cluster cluster;                /* cluster that violate the FD */
    size_t num_different_rhs_values;           /* number of different RHS values within a cluster */
    double most_frequent_rhs_value_proportion; /* proportion of most frequent RHS value */
public:
    util::PLI::Cluster const& GetCluster() const {
        return cluster;
    }

    size_t GetNumDifferentRhsValues() const {
        return num_different_rhs_values;
    }

    double GetMostFrequentValueProportion() const {
        return most_frequent_rhs_value_proportion;
    }

    Highlight(util::PLI::Cluster const& cluster, size_t num_different_rhs_values,
              size_t num_most_frequent_rhs_value)
        : cluster(cluster),
          num_different_rhs_values(num_different_rhs_values),
          most_frequent_rhs_value_proportion((double)num_most_frequent_rhs_value / cluster.size()) {
    }
};

}  // namespace algos::fd_verifier
