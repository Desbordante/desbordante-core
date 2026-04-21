#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "core/config/column_index/type.h"
#include "core/config/common_option.h"
#include "core/config/custom_metric/custom_metrics/type.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option.h"

namespace config {
/// @brief Option for a collection of user-defined metrics
class MetricsOption {
private:
    CommonOption<CustomMetricsType> const common_option_;

    static void CheckMetrics(CustomMetricsType const& value, std::size_t indices_count) {
        if (value.size() > indices_count) {
            throw ConfigurationError("Too many user-defined metrics");
        }
    }

    static void NormalizeMetrics(CustomMetricsType& value, std::size_t indices_count) {
        // A single static instance could be used, but Python would try to destruct it
        auto default_metric = std::make_shared<util::DefaultCustomMetric>();

        std::ranges::replace_if(value, std::logical_not{}, default_metric);
        value.resize(indices_count, default_metric);
    }

public:
    MetricsOption(std::string_view name = names::kCustomMetrics,
                  std::string_view description = descriptions::kDCustomMetrics)
        : common_option_(name, description, {}) {}

    // NOTE: This option should depend on indices option (see @c SetConditionalOpts)
    // to properly get column count
    [[nodiscard]] Option<CustomMetricsType> operator()(
            CustomMetricsType* value_ptr, std::function<IndexType()> get_col_count) const;
};
}  // namespace config
