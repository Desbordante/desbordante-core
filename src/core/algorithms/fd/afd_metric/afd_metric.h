#pragma once

#include "core/util/better_enum_with_visibility.h"

namespace algos::afd_metric_calculator {

BETTER_ENUM(AFDMetric, char, g2 = 0, tau, mu_plus, fi, rho, g1, g3, pdep)

}  // namespace algos::afd_metric_calculator
