#include "core/config/custom_metric/custom_metric/option.h"

#include <variant>

#include "core/config/custom_metric/custom_metric/type.h"
#include "core/config/option.h"
#include "core/util/custom_metric/custom_metric.h"

namespace config {

namespace {
void Normalize(CustomMetricType& value) {
    auto valueless = [](auto const& p) { return p == nullptr; };
    if (std::visit(valueless, value)) {
        value = std::make_shared<util::DefaultCustomMetric>();
    }
}
}  // namespace

Option<CustomMetricType> MetricOption(CustomMetricType* value_ptr, std::string_view name,
                                      std::string_view description) {
    Option<CustomMetricType> option{value_ptr, name, description,
                                    std::shared_ptr<util::ICustomMetric>{nullptr}};
    option.SetNormalizeFunc(&Normalize);
    return option;
}
}  // namespace config
