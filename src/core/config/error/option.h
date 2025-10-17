#pragma once

#include "config/common_option.h"
#include "config/error/type.h"

namespace config {
template <typename T>
class CommonOption;

extern CommonOption<ErrorType> const kErrorOpt;

}  // namespace config
