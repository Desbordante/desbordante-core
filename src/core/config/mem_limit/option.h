#pragma once

#include "core/config/common_option.h"
#include "core/config/mem_limit/type.h"

namespace config {
extern CommonOption<MemLimitMBType> const kMemLimitMbOpt;
}  // namespace config
