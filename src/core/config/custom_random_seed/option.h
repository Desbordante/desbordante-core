#pragma once

#include "config/custom_random_seed/type.h"  // for CustomRandomSeedType

namespace config {
template <typename T>
class CommonOption;
}

namespace config {
extern CommonOption<CustomRandomSeedType> const kCustomRandomFlagOpt;
}  // namespace config
