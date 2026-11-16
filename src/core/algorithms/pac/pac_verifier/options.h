#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/algorithms/pac/pac_verifier/util/validation_mode.h"
#include "core/config/common_option.h"
#include "core/config/option.h"

namespace algos::pac_verifier {
config::Option<PACValidationMode> PACValidationModeOption(
        PACValidationMode* value_ptr,
        std::unordered_map<PACValidationMode, std::vector<std::string_view>> conditional_options);

extern config::CommonOption<double> const kMinEpsilonOption;

config::CommonOption<double> MaxEpsilonOption(std::function<double()> get_min_epsilon);

config::CommonOption<double> MinDeltaOption(std::function<double()> get_min_epsilon,
                                            std::function<double()> get_max_epsilon);

config::CommonOption<double> MaxDeltaOption(std::function<double()> get_min_epsilon,
                                            std::function<double()> get_max_epsilon);

config::CommonOption<unsigned long> DeltaStepsOption(std::function<double()> get_min_delta,
                                                     std::function<double()> get_max_delta);

extern config::CommonOption<double> const kDeltaOption;
extern config::CommonOption<double> const kEpsilonOption;

extern config::CommonOption<double> const kDiagonalThresholdOption;
}  // namespace algos::pac_verifier
