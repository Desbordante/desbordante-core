#pragma once

#include "core/config/common_option.h"
#include "core/config/error_measure/type.h"

namespace config {

extern CommonOption<PfdErrorMeasureType> const kPfdErrorMeasureOpt;
extern CommonOption<AfdErrorMeasureType> const kAfdErrorMeasureOpt;
}  // namespace config
