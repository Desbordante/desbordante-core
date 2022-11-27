#pragma once

#include <limits>

#include "algorithms/options/descriptions.h"
#include "algorithms/options/names.h"
#include "algorithms/options/type.h"

namespace algos::config {

using MaxLhsType = unsigned int;
static const config::OptionType<MaxLhsType> MaxLhsOpt{
        {config::names::kMaximumLhs, config::descriptions::kDMaximumLhs},
        std::numeric_limits<MaxLhsType>::max()
};

}  // namespace algos::config
