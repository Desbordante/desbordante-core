#pragma once

#include "config/equal_nulls/type.h"  // for EqNullsType

namespace config {
template <typename T>
class CommonOption;
}

namespace config {
extern CommonOption<EqNullsType> const kEqualNullsOpt;
}  // namespace config
