#pragma once

#include <memory>
#include <variant>

#include "core/util/custom_metric/custom_metric.h"

namespace config {
using CustomMetricType = std::variant<std::shared_ptr<util::ICustomMetric>,
                                      std::shared_ptr<util::IMetrizableCustomMetric>>;
}  // namespace config
