#pragma once

#include "core/util/better_enum_with_visibility.h"

namespace algos::hpiv::timer {

BETTER_ENUM(TimerName, char, total = 0, construct_clusters, total_enum_algo, sample_diff_sets,
            cluster_intersect, num_of_timers);

}  // namespace algos::hpiv::timer
