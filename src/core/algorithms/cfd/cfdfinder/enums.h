#pragma once

#include <enum.h>

namespace algos::cfdfinder {

BETTER_ENUM(Pruning, char, legacy = 0, support_independent, partial_fd, rhs_filter);
BETTER_ENUM(Expansion, char, constant = 0, range, negative_constant);

}  // namespace algos::cfdfinder
