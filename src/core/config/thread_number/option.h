#pragma once

#include "config/thread_number/type.h"  // for ThreadNumType

namespace config {
template <typename T>
class CommonOption;
}

namespace config {
extern CommonOption<ThreadNumType> const kThreadNumberOpt;
}  // namespace config
