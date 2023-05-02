#pragma once

#include <thread>

#include "algorithms/options/common_option.h"
#include "algorithms/options/thread_number/type.h"

namespace algos::config {
extern const CommonOption<ThreadNumType> ThreadNumberOpt;
}  // namespace algos::config
