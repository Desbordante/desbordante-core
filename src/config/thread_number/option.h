#pragma once

#include <thread>

#include "config/common_option.h"
#include "config/thread_number/type.h"

namespace util::config {
extern const CommonOption<ThreadNumType> ThreadNumberOpt;
}  // namespace util::config
