#pragma once

#include "config/common_option.h"
#include "config/mem_limit/type.h"

namespace config {
extern CommonOption<MemLimitMBType> const kMemLimitMbOpt;
}  // namespace config
