#pragma once

#include "algorithms/options/descriptions.h"
#include "algorithms/options/names.h"
#include "algorithms/options/type.h"

namespace algos::config {
using EqNullsType = bool;
const OptionType<EqNullsType> EqualNullsOpt{{names::kEqualNulls, descriptions::kDEqualNulls}, true};
}  // namespace algos::config
