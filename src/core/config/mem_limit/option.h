#pragma once

#include "config/common_option.h"
#include "config/mem_limit/type.h"

namespace config {
template <typename T>
class CommonOption;

extern CommonOption<MemLimitMBType> const kMemLimitMbOpt;
}  // namespace config
