#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::hpiv::timer {

enum class TimerName : size_t {
    kTotal = 0,
    kConstructClusters,
    kTotalEnumAlgo,
    kSampleDiffSets,
    kClusterIntersect,
    kNumOfTimers
};
}  // namespace algos::hpiv::timer
