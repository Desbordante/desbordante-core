#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::afd_metric_calculator {

enum class AFDMetric : char { kG2 = 0, kTau, kMuPlus, kFi };

}  // namespace algos::afd_metric_calculator
