#include "core/config/time_limit/option.h"

#include "core/config/names_and_descriptions.h"

namespace config {
using names::kTimeLimitSeconds, descriptions::kDTimeLimitSeconds;
extern CommonOption<TimeLimitSecondsType> const kTimeLimitSecondsOpt{kTimeLimitSeconds,
                                                                     kDTimeLimitSeconds, 0u};
}  // namespace config
