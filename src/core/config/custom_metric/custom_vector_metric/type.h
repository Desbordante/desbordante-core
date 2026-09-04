#pragma once

#include <memory>

#include "core/util/custom_metric/custom_vector_metric.h"

namespace config {
using CustomVectorMetricOptionType = std::shared_ptr<util::ICustomVectorMetric>;
using CustomVectorMetricType = std::unique_ptr<util::ICustomVectorMetric::ITypedMetric>;
}  // namespace config
