#include "config/time_limit/option.h"

#include <variant>  // for variant

#include "common_option.h"  // for CommonOption
#include "config/names_and_descriptions.h"
#include "time_limit/type.h"  // for TimeLimitSecondsType

namespace config {
using names::kTimeLimitSeconds, descriptions::kDTimeLimitSeconds;
extern CommonOption<TimeLimitSecondsType> const kTimeLimitSecondsOpt{kTimeLimitSeconds,
                                                                     kDTimeLimitSeconds, 0u};
}  // namespace config
