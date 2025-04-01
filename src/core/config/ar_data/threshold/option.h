#pragma once

#include "config/ar_data/threshold/type.h"
#include "config/common_option.h"

namespace config {

extern CommonOption<ArThresholdType> const kMinimumSupportOpt;
extern CommonOption<ArThresholdType> const kMinimumConfidenceOpt;
}  // namespace config
