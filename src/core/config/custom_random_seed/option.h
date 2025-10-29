#pragma once

#include "config/common_option.h"
#include "config/custom_random_seed/type.h"

namespace config {
template <typename T>
class CommonOption;

extern CommonOption<CustomRandomSeedType> const kCustomRandomFlagOpt;
}  // namespace config
