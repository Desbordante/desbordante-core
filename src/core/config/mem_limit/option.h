#pragma once

#include "config/mem_limit/type.h"  // for MemLimitMBType

namespace config {
template <typename T>
class CommonOption;
}

namespace config {
extern CommonOption<MemLimitMBType> const kMemLimitMbOpt;
}  // namespace config
