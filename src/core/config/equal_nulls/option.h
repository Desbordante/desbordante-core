#pragma once

#include "config/common_option.h"
#include "config/equal_nulls/type.h"

namespace config {
template <typename T>
class CommonOption;

extern CommonOption<EqNullsType> const kEqualNullsOpt;
}  // namespace config
