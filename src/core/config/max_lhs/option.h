#pragma once

#include "config/common_option.h"
#include "config/max_lhs/type.h"

namespace config {
template <typename T>
class CommonOption;

extern CommonOption<MaxLhsType> const kMaxLhsOpt;
}  // namespace config
