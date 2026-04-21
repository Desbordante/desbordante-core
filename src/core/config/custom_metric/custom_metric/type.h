#pragma once

#include <memory>

#include "core/util/custom_metric/custom_metric.h"

namespace config {
using CustomMetricType = std::shared_ptr<util::ICustomMetric>;
}  // namespace config
