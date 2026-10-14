#pragma once

#include <memory>

#include "core/util/custom_metric/custom_metric.h"

namespace config {
using CustomMetricOptionType = std::shared_ptr<util::ICustomMetric>;
using CustomMetricType = std::unique_ptr<util::ICustomMetric::ITypedMetric>;
}  // namespace config
