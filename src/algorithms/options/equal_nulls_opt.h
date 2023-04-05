#pragma once

#include "algorithms/options/names_and_descriptions.h"
#include "algorithms/options/type.h"

namespace algos::config {
using EqNullsType = bool;
const OptionType<EqNullsType> EqualNullsOpt{{names::kEqualNulls, descriptions::kDEqualNulls}, true};
}  // namespace algos::config
