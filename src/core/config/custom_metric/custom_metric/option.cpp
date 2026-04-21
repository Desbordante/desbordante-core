#include "core/config/custom_metric/custom_metric/option.h"

#include "core/config/custom_metric/custom_metric/type.h"
#include "core/config/option.h"

namespace config {

namespace {
void Normalize(CustomMetricType& value) {
    if (!value) {
        value = std::make_shared<util::DefaultCustomMetric>();
    }
}
}  // namespace

Option<CustomMetricType> MetricOption(CustomMetricType* value_ptr, std::string_view name,
                                      std::string_view description) {
    Option<CustomMetricType> option{value_ptr, name, description, CustomMetricType{nullptr}};
    option.SetNormalizeFunc(&Normalize);
    return option;
}
}  // namespace config
