#pragma once

#include <thread>

#include "core/config/common_option.h"
#include "core/config/thread_number/type.h"

namespace config {
extern CommonOption<ThreadNumType> const kThreadNumberOpt;
}  // namespace config
