#pragma once

#include "config/common_option.h"
#include "config/error_measure/type.h"

namespace config {

extern CommonOption<PfdErrorMeasureType> const kPfdErrorMeasureOpt;
extern CommonOption<AfdErrorMeasureType> const kAfdErrorMeasureOpt;
extern CommonOption<ADCErrorMeasureType> const kADCErrorMeasureOpt;
}  // namespace config
