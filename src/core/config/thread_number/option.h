#pragma once

#include <thread>

#include "config/common_option.h"
#include "config/thread_number/type.h"

namespace config {
extern CommonOption<ThreadNumType> const ThreadNumberOpt;
}  // namespace config
