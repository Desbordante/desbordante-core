#pragma once

#include <string_view>

#include "core/config/custom_metric/custom_vector_metric/type.h"
#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option.h"

namespace config {
Option<CustomVectorMetricType> VectorMetricOption(
        CustomVectorMetricType* value_ptr, std::string_view name = names::kCustomMetric,
        std::string_view description = descriptions::kDCustomMetric);
}  // namespace config
