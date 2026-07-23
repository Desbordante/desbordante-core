#pragma once

#include "core/algorithms/fd/hycommon/efficiency_threshold.h"

namespace algos::fd::hyfd {

class HyFDConfig {
public:
    static constexpr double kEfficiencyThreshold = hy::kEfficiencyThreshold;
};

}  // namespace algos::fd::hyfd
