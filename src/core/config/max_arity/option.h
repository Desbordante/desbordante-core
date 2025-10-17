#pragma once

#include "config/common_option.h"
#include "config/max_arity/type.h"

namespace config {
template <typename T>
class CommonOption;

extern CommonOption<MaxArityType> const kMaxArityOpt;
}  // namespace config
