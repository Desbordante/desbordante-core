#pragma once

#include "config/common_option.h"
#include "config/thread_number/type.h"

namespace config {
template <typename T>
class CommonOption;

extern CommonOption<ThreadNumType> const kThreadNumberOpt;
}  // namespace config
