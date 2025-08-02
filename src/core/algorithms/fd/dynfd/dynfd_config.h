#pragma once

namespace algos::dynfd {

class DynFDConfig {
public:
    static constexpr double kInvalidatedFdsThreshold = 0.1;
    static constexpr double kInvalidatedNonFdsThreshold = 0.1;
    static constexpr double kSamplingEfficiencyThreshold = 0.1;
    static constexpr double kDfsSeedSampleRatio = 0.1;
};

}  // namespace algos::hyfd
