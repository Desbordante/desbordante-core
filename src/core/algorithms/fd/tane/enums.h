#pragma once

#include "core/util/better_enum_with_visibility.h"

namespace algos {
BETTER_ENUM(PfdErrorMeasure, char, per_tuple = 0, per_value)

BETTER_ENUM(AfdErrorMeasure, char, g1 = 0, pdep, tau, mu_plus, rho, fi, g2, g3)
}  // namespace algos
