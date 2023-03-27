#pragma once

#include <limits>

#include "algorithms/options/common_option.h"
#include "algorithms/options/names_and_descriptions.h"

namespace algos::config {

using MaxLhsType = unsigned int;
static const config::CommonOption<MaxLhsType> MaxLhsOpt{
        {config::names::kMaximumLhs, config::descriptions::kDMaximumLhs},
        std::numeric_limits<MaxLhsType>::max()
};

}  // namespace algos::config
