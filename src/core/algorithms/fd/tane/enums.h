#pragma once

#include <enum.h>

namespace algos {
BETTER_ENUM(PfdErrorMeasure, char, per_tuple = 0, per_value)

BETTER_ENUM(AfdErrorMeasure, char, g1 = 0, pdep, tau, mu_plus, rho)
}  // namespace algos
