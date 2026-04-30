#pragma once

#include "core/util/better_enum_with_visibility.h"

namespace algos::cind {
BETTER_ENUM(CondType, char, row = 0, group)
BETTER_ENUM(AlgoType, char, cinderella = 0, pli_cind)
}  // namespace algos::cind
