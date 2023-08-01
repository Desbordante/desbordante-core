#pragma once

#include <thread>

#include "config/common_option.h"
#include "config/thread_number/type.h"

namespace config {
extern const CommonOption<ThreadNumType> ThreadNumberOpt;
}  // namespace config
