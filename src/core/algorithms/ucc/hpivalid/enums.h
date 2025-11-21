#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::hpiv::timer {

enum class TimerName : size_t {
    total = 0,
    construct_clusters,
    total_enum_algo,
    sample_diff_sets,
    cluster_intersect,
    num_of_timers
};
}  // namespace algos::hpiv::timer
