#pragma once

#include "config/max_lhs/type.h"  // for MaxLhsType

namespace config {
template <typename T>
class CommonOption;
}

namespace config {
extern CommonOption<MaxLhsType> const kMaxLhsOpt;
}  // namespace config
