#pragma once

#include "config/max_arity/type.h"  // for MaxArityType

namespace config {
template <typename T>
class CommonOption;
}

namespace config {
extern CommonOption<MaxArityType> const kMaxArityOpt;
}  // namespace config
