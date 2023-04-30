#pragma once

#include <thread>

#include "util/config/common_option.h"
#include "util/config/thread_number/type.h"

namespace util::config {
extern const CommonOption<ThreadNumType> ThreadNumberOpt;
}  // namespace util::config
