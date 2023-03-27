#pragma once

#include "algorithms/options/common_option.h"
#include "algorithms/options/names_and_descriptions.h"

namespace algos::config {
using EqNullsType = bool;
const CommonOption<EqNullsType> EqualNullsOpt{
        {names::kEqualNulls, descriptions::kDEqualNulls}, true};
}  // namespace algos::config
