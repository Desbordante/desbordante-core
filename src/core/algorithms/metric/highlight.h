#pragma once

#include "aliases.h"

namespace algos::metric {

struct Highlight {
    ClusterIndex data_index;
    ClusterIndex furthest_data_index;
    long double max_distance;

    Highlight(ClusterIndex data_index, ClusterIndex furthest_data_index, long double max_distance)
        : data_index(data_index),
          furthest_data_index(furthest_data_index),
          max_distance(max_distance) {}

    std::tuple<ClusterIndex, ClusterIndex, long double> ToTuple() const {
        return {data_index, furthest_data_index, max_distance};
    }
};

}  // namespace algos::metric
