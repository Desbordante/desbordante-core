#pragma once

#include <functional>

#include "core/config/common_option.h"

namespace algos::pac_verifier {
extern config::CommonOption<double> const kMinEpsilonOption;

config::CommonOption<double> MaxEpsilonOption(std::function<double()> get_min_epsilon);

config::CommonOption<double> MinDeltaOption(std::function<double()> get_min_epsilon,
                                            std::function<double()> get_max_epsilon);

config::CommonOption<double> MaxDeltaOption(std::function<double()> get_min_epsilon,
                                            std::function<double()> get_max_epsilon);

config::CommonOption<unsigned long> DeltaStepsOption(std::function<double()> get_min_delta,
                                                     std::function<double()> get_max_delta);

extern config::CommonOption<double> const kDiagonalThresholdOption;
}  // namespace algos::pac_verifier
