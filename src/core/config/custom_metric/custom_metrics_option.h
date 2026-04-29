#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "core/config/custom_metric/custom_metric.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option.h"

namespace config {
/// @brief Option for a collection of user-defined metrics
class MetricsOption : public Option<std::vector<std::shared_ptr<ICustomMetric>>> {
private:
    using ValueType = std::vector<std::shared_ptr<ICustomMetric>>;

public:
    MetricsOption(ValueType* value_ptr, std::string_view name = names::kCustomMetrics,
                  std::string_view description = descriptions::kDCustomMetrics)
        : Option(value_ptr, name, description, ValueType{}) {}

    /// @brief Substitute default metrics for columns on which metrics are not specified
    /// This function should be called after indices are set (most probably, at the very beginning
    /// of @c LoadData)
    /// Alternatively, you can make this option depend on indices (with @c SetConditionalOption) and
    /// set this function as a normalization function
    static void NormalizeMetrics(ValueType& value, std::size_t indices_count) {
        // A single static instance could be used, but Python would try to destruct it
        auto default_metric = std::make_shared<DefaultCustomMetric>();

        if (value.size() > indices_count) {
            throw ConfigurationError("Too many user-defined metrics");
        }
        std::ranges::replace_if(value, std::logical_not{}, default_metric);
        value.resize(indices_count, default_metric);
    }
};
}  // namespace config
