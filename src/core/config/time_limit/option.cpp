#include "config/time_limit/option.h"

#include <variant>

#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "time_limit/type.h"

namespace config {
using names::kTimeLimitSeconds, descriptions::kDTimeLimitSeconds;
extern CommonOption<TimeLimitSecondsType> const kTimeLimitSecondsOpt{kTimeLimitSeconds,
                                                                     kDTimeLimitSeconds, 0u};
}  // namespace config
