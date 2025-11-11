#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos {
enum class PfdErrorMeasure : char { per_tuple = 0, per_value };

enum class AfdErrorMeasure : char { g1 = 0, pdep, tau, mu_plus, rho };
}  // namespace algos
