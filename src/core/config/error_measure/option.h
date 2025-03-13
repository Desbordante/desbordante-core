#pragma once

#include "config/error_measure/type.h"  // for AfdErrorMeasureType, PfdError...

namespace config {
template <typename T>
class CommonOption;
}

namespace config {

extern CommonOption<PfdErrorMeasureType> const kPfdErrorMeasureOpt;
extern CommonOption<AfdErrorMeasureType> const kAfdErrorMeasureOpt;
}  // namespace config
