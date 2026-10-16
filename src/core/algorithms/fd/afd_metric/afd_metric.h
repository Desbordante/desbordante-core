#pragma once

#include "core/util/export.h"

namespace algos::afd_metric_calculator {

enum class DESBORDANTE_EXPORT AFDMetric : char {
    kG2 = 0,
    kTau,
    kMuPlus,
    kFi,
    kG1,
    kG3,
    kPdep,
    kRho,
    kRfiPlus,
    kG1S,
    kRfiPrimePlus
};
}  // namespace algos::afd_metric_calculator
