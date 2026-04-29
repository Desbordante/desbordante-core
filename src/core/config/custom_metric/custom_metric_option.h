#pragma once

#include <memory>
#include <string_view>

#include "core/config/custom_metric/custom_metric.h"
#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option.h"

namespace config {
/// @brief An option that represents a custom metric on a single column in the table
class MetricOption : public Option<std::shared_ptr<ICustomMetric>> {
private:
    using ValueType = std::shared_ptr<ICustomMetric>;

    static void Normalize(ValueType& value) {
        if (!value) {
            value = std::make_shared<DefaultCustomMetric>();
        }
    }

public:
    MetricOption(std::shared_ptr<ICustomMetric>* value_ptr,
                 std::string_view name = names::kCustomMetric,
                 std::string_view description = descriptions::kDCustomMetric)
        : Option(value_ptr, name, description, ValueType{nullptr}) {
        // User can pass `nullptr` to explicitly say that they want to use default metric
        SetNormalizeFunc(&Normalize);
    }
};
}  // namespace config
