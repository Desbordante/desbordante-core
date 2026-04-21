#pragma once

#include <memory>

#include "core/util/custom_metric/custom_vector_metric.h"

namespace config {
using CustomVectorMetricType = std::shared_ptr<util::ICustomVectorMetric>;
}  // namespace config
