#pragma once

#include <cstddef>

#include "model/table/position_list_index.h"

namespace algos::fd_verifier {

/* FDVerifier Highlight represents a cluster that violate the FD and provides the information about
 * that cluster */
class Highlight {
private:
    model::PLI::Cluster cluster;               /* cluster that violate the FD */
    size_t num_distinct_rhs_values;            /* number of different RHS values within a cluster */
    double most_frequent_rhs_value_proportion; /* proportion of most frequent RHS value */
public:
    model::PLI::Cluster const& GetCluster() const {
        return cluster;
    }

    size_t GetNumDistinctRhsValues() const {
        return num_distinct_rhs_values;
    }

    double GetMostFrequentRhsValueProportion() const {
        return most_frequent_rhs_value_proportion;
    }

    Highlight(model::PLI::Cluster const& cluster, size_t num_distinct_rhs_values,
              size_t num_most_frequent_rhs_value)
        : cluster(cluster),
          num_distinct_rhs_values(num_distinct_rhs_values),
          most_frequent_rhs_value_proportion((double)num_most_frequent_rhs_value / cluster.size()) {
    }
};

}  // namespace algos::fd_verifier
