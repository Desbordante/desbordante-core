#pragma once

#include <cstddef>

#include "model/table/position_list_index.h"

namespace algos::fd_verifier {

/* FDVerifier Highlight represents a cluster that violate the FD and provides the information about
 * that cluster */
class Highlight {
private:
    model::PLI::Cluster cluster_;    /* cluster that violate the FD */
    size_t num_distinct_rhs_values_; /* number of different RHS values within a cluster */
    double most_frequent_rhs_value_proportion_; /* proportion of most frequent RHS value */
public:
    model::PLI::Cluster const& GetCluster() const {
        return cluster_;
    }

    size_t GetNumDistinctRhsValues() const {
        return num_distinct_rhs_values_;
    }

    double GetMostFrequentRhsValueProportion() const {
        return most_frequent_rhs_value_proportion_;
    }

    Highlight(model::PLI::Cluster const& cluster, size_t num_distinct_rhs_values,
              size_t num_most_frequent_rhs_value)
        : cluster_(cluster),
          num_distinct_rhs_values_(num_distinct_rhs_values),
          most_frequent_rhs_value_proportion_((double)num_most_frequent_rhs_value /
                                              cluster.size()) {}
};

}  // namespace algos::fd_verifier
