#pragma once

#include <memory>
#include <vector>

#include "core/util/custom_metric/custom_metric.h"

namespace config {
using CustomMetricsType = std::vector<std::shared_ptr<util::ICustomMetric>>;
}  // namespace config
