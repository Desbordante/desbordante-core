#pragma once

#include "config/time_limit/type.h"  // for TimeLimitSecondsType

namespace config {
template <typename T>
class CommonOption;
}

namespace config {
extern CommonOption<TimeLimitSecondsType> const kTimeLimitSecondsOpt;
}  // namespace config
