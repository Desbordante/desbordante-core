#pragma once

#include "core/config/common_option.h"
#include "core/config/custom_random_seed/type.h"

namespace config {
extern CommonOption<CustomRandomSeedType> const kCustomRandomFlagOpt;
}  // namespace config
