#pragma once

#include "config/error/type.h"  // for ErrorType

namespace config {
template <typename T>
class CommonOption;
}

namespace config {

extern CommonOption<ErrorType> const kErrorOpt;

}  // namespace config
