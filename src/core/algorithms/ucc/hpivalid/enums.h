#pragma once

#include <enum.h>

namespace algos::hpiv::timer {

BETTER_ENUM(TimerName, char, total = 0, total_preprocessing, read_table, create_subtable,
            construct_clusters, total_enum_algo, sample_diff_sets, cluster_intersect,
            num_of_timers);

}  // namespace algos::hpiv::timer
