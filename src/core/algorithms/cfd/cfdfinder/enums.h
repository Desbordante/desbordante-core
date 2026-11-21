#pragma once

#include "core/util/export.h"

namespace algos::cfdfinder {
enum class Pruning : char { kLegacy = 0, kSupportIndependent, kPartialFd };
enum class Expansion : char { kConstant = 0, kRange, kNegativeConstant };
enum class Result : char { kDirect = 0, kLattice, kTree };

}  // namespace algos::cfdfinder
